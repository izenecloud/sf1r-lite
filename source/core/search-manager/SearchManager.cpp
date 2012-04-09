#include <bundles/index/IndexBundleConfiguration.h>
#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>
#include <query-manager/ActionItem.h>
#include <ranking-manager/RankingManager.h>
#include <mining-manager/MiningManager.h>
#include <mining-manager/faceted-submanager/ctr_manager.h>
#include <mining-manager/faceted-submanager/GroupFilterBuilder.h>
#include <mining-manager/faceted-submanager/GroupFilter.h>
#include <mining-manager/faceted-submanager/ontology_rep.h>
#include <aggregator-manager/Notifier.h>
#include <common/SFLogger.h>

#include "SearchManager.h"
#include "CustomRanker.h"
#include "QueryBuilder.h"
#include "Sorter.h"
#include "HitQueue.h"
#include "FilterDocumentIterator.h"
#include "AllDocumentIterator.h"
#include "CombinedDocumentIterator.h"

#include <util/swap.h>
#include <util/get.h>

#include <boost/algorithm/string.hpp>

#include <fstream>
#include <algorithm>

namespace sf1r
{

const std::string RANK_PROPERTY("_rank");
const std::string DATE_PROPERTY("date");
const std::string CUSTOM_RANK_PROPERTY("custom_rank");
const std::string CTR_PROPERTY("_ctr");

bool hasRelevenceSort(const std::pair<std::string , bool>& element)
{
    std::string fieldNameL = element.first;
    boost::to_lower(fieldNameL);
    if (fieldNameL == RANK_PROPERTY)
        return true;
    return false;
}

bool action_rerankable(const KeywordSearchActionItem& actionItem)
{
    if (actionItem.searchingMode_.mode_ == SearchingMode::KNN)
        return false;

    bool rerank = false;
    if (actionItem.env_.taxonomyLabel_.empty() &&
            actionItem.env_.nameEntityItem_.empty() &&
            actionItem.groupParam_.groupLabels_.empty() &&
            actionItem.groupParam_.attrLabels_.empty())
    {
        const std::vector<std::pair<std::string , bool> >& sortPropertyList
            = actionItem.sortPriorityList_;
        if (sortPropertyList.empty())
            rerank = true;
        else if (sortPropertyList.size() == 1)
        {
            std::string fieldNameL = sortPropertyList[0].first;
            boost::to_lower(fieldNameL);
            if (fieldNameL == RANK_PROPERTY)
                rerank = true;
        }
        else
        {
            std::vector<std::pair<std::string , bool> >::const_iterator it
                = std::find_if(sortPropertyList.begin(), sortPropertyList.end(), hasRelevenceSort);
            if (it != sortPropertyList.end())
                rerank = true;
        }
    }
    return rerank;
}

SearchManager::SearchManager(
        const IndexBundleSchema& indexSchema,
        const boost::shared_ptr<IDManager>& idManager,
        const boost::shared_ptr<DocumentManager>& documentManager,
        const boost::shared_ptr<IndexManager>& indexManager,
        const boost::shared_ptr<RankingManager>& rankingManager,
        IndexBundleConfiguration* config
)
    : config_(config)
    , schemaMap_()
    , indexManagerPtr_(indexManager)
    , documentManagerPtr_(documentManager)
    , rankingManagerPtr_(rankingManager)
    , queryBuilder_()
    , filter_hook_(0)
    , reranker_(0)
{
    collectionName_ = config->collectionName_;
    for (IndexBundleSchema::const_iterator iter = indexSchema.begin();
        iter != indexSchema.end(); ++iter)
    {
        schemaMap_[iter->getName()] = *iter;
    }

    pSorterCache_ = new SortPropertyCache(indexManagerPtr_.get(), config);
    queryBuilder_.reset(new QueryBuilder(
                            indexManager,
                            documentManager,
                            idManager,
                            schemaMap_,
                            config->filterCacheNum_));
    rankingManagerPtr_->getPropertyWeightMap(propertyWeightMap_);
}

SearchManager::~SearchManager()
{
    if (pSorterCache_)
        delete pSorterCache_;
}

void SearchManager::reset_cache(
        bool rType,
        docid_t id,
        const std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue)
{
    //this method is only used for r-type filed right now
    if (!rType)
    {
        //pSorterCache_->setDirty(true);
        return;
    }
    else
    {
        pSorterCache_->updateSortData(id, rTypeFieldValue);
        return;
    }


    // notify master
    NotifyMSG msg;
    msg.identity = collectionName_;
    msg.method = "clear_search_cache";
    Notifier::get()->notify(msg);

    queryBuilder_->reset_cache();
}

void SearchManager::reset_all_property_cache()
{
    pSorterCache_->setDirty(true);

    // notify master
    NotifyMSG msg;
    msg.identity = collectionName_;
    msg.method = "clear_search_cache";
    Notifier::get()->notify(msg);

    queryBuilder_->reset_cache();
}

void SearchManager::setGroupFilterBuilder(
    faceted::GroupFilterBuilder* builder)
{
    groupFilterBuilder_.reset(builder);
}

void SearchManager::setMiningManager(
        boost::shared_ptr<MiningManager> miningManagerPtr)
{
    miningManagerPtr_ = miningManagerPtr;
}

NumericPropertyTable*
SearchManager::createPropertyTable(const std::string& propertyName)
{
    boost::shared_ptr<PropertyData> propData = getPropertyData_(propertyName);
    if (propData)
    {
        return new NumericPropertyTable(propertyName, propData);
    }

    return NULL;
}

bool SearchManager::rerank(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    if (reranker_ && resultItem.topKCustomRankScoreList_.empty() && action_rerankable(actionItem))
    {
        reranker_(resultItem.topKDocs_, resultItem.topKRankScoreList_, actionItem.env_.queryString_);
        return true;
    }
    return false;
}

bool SearchManager::search(
        SearchKeywordOperation& actionOperation,
        std::vector<unsigned int>& docIdList,
        std::vector<float>& rankScoreList,
        std::vector<float>& customRankScoreList,
        std::size_t& totalCount,
        faceted::GroupRep& groupRep,
        faceted::OntologyRep& attrRep,
        sf1r::PropertyRange& propertyRange,
        DistKeywordSearchInfo& distSearchInfo,
        uint32_t topK,
        uint32_t knnTopK,
        uint32_t knnDist,
        uint32_t start)
{
    CREATE_PROFILER( preparedociter, "SearchManager", "doSearch_: SearchManager_search : build doc iterator");
    CREATE_PROFILER( preparerank, "SearchManager", "doSearch_: prepare ranker");
    CREATE_PROFILER( preparesort, "SearchManager", "doSearch_: prepare sort");

    unsigned int collectionId = 1;
    std::vector<std::string> indexPropertyList( actionOperation.actionItem_.searchPropertyList_ );
#if PREFETCH_TERMID
    std::stable_sort (indexPropertyList.begin(), indexPropertyList.end());
#endif
    unsigned indexPropertySize = indexPropertyList.size();
    std::vector<propertyid_t> indexPropertyIdList(indexPropertySize);
    std::transform(
            indexPropertyList.begin(),
            indexPropertyList.end(),
            indexPropertyIdList.begin(),
            boost::bind(&QueryBuilder::getPropertyIdByName, queryBuilder_.get(), _1)
    );

    if ( actionOperation.noError() == false )
        return false;

    START_PROFILER( preparedociter )
    sf1r::TextRankingType& pTextRankingType = actionOperation.actionItem_.rankingType_;
    bool isWandStrategy = (actionOperation.actionItem_.searchingMode_.mode_ == SearchingMode::WAND);
    bool isTFIDFModel = (pTextRankingType == RankingType::BM25
                      ||config_->rankingManagerConfig_.rankingConfigUnit_.textRankingModel_ == RankingType::BM25);
    bool isWandSearch = (isWandStrategy && isTFIDFModel && (!actionOperation.isPhraseOrWildcardQuery_));

    // references for property term info
    const property_term_info_map& propertyTermInfoMap =
        actionOperation.getPropertyTermInfoMap();
    // use empty object for not found property
    const PropertyTermInfo emptyPropertyTermInfo;

    // build term index maps
    std::vector<std::map<termid_t, unsigned> > termIndexMaps(indexPropertySize);
    typedef std::vector<std::string>::const_iterator property_list_iterator;
    for (uint32_t i = 0; i < indexPropertyList.size(); ++i)
    {
        const PropertyTermInfo::id_uint_list_map_t& termPositionsMap =
            izenelib::util::getOr(
                    propertyTermInfoMap,
                    indexPropertyList[i],
                    emptyPropertyTermInfo
            ).getTermIdPositionMap();

        unsigned index = 0;
        typedef PropertyTermInfo::id_uint_list_map_t::const_iterator
        term_id_position_iterator;
        for (term_id_position_iterator termIt = termPositionsMap.begin();
                termIt != termPositionsMap.end(); ++termIt)
        {
            termIndexMaps[i][termIt->first] = index++;
        }
    }

    std::vector<boost::shared_ptr<PropertyRanker> > propertyRankers;
    rankingManagerPtr_->createPropertyRankers(pTextRankingType, indexPropertySize, propertyRankers);
    bool readTermPosition = propertyRankers[0]->requireTermPosition();

    boost::scoped_ptr<CombinedDocumentIterator> pDocIterator(new CombinedDocumentIterator());
    MultiPropertyScorer* pMultiPropertyIterator = NULL;
    WANDDocumentIterator* pWandDocIterator = NULL;
    std::vector<QueryFiltering::FilteringType>& filtingList
    = actionOperation.actionItem_.filteringList_;
    boost::shared_ptr<EWAHBoolArray<uint32_t> > pFilterIdSet;
    try
    {
        if (filter_hook_)
            filter_hook_(filtingList);
        if (!filtingList.empty())
            queryBuilder_->prepare_filter(filtingList, pFilterIdSet);

        if (actionOperation.rawQueryTree_->type_ != QueryTree::FILTER_QUERY)
        {
            if( isWandSearch )
            {
                pWandDocIterator = queryBuilder_->prepare_wand_dociterator(
                                        actionOperation,
                                        collectionId,
                                        propertyWeightMap_,
                                        indexPropertyList,
                                        indexPropertyIdList,
                                        readTermPosition,
                                        termIndexMaps
                                   );
            }
            else
            {
                pMultiPropertyIterator = queryBuilder_->prepare_dociterator(
                                             actionOperation,
                                             collectionId,
                                             propertyWeightMap_,
                                             indexPropertyList,
                                             indexPropertyIdList,
                                             readTermPosition,
                                             termIndexMaps
                                         );
            }

            if ( !pMultiPropertyIterator && !pWandDocIterator )
            {
                return false;
            }
        }

    }
    catch (std::exception& e)
    {
        if (pMultiPropertyIterator)
            delete pMultiPropertyIterator;
        if(pWandDocIterator)
            delete pWandDocIterator;
        return false;
    }

    if (pFilterIdSet)
    {
        ///1. Search Filter
        ///2. Select * WHERE    (FilterQuery)
        BitMapIterator* pBitmapIter = new BitMapIterator(pFilterIdSet);
        FilterDocumentIterator* pFilterIterator = new FilterDocumentIterator(pBitmapIter);
        pDocIterator->add((DocumentIterator*) pFilterIterator);
    }

    if (pMultiPropertyIterator)
    {
        ///Relevance query
        pDocIterator->add((DocumentIterator*) pMultiPropertyIterator);
    }
    else if (pWandDocIterator)
    {
        pDocIterator->add((DocumentIterator*) pWandDocIterator);
    }

    STOP_PROFILER( preparedociter )

    START_PROFILER( preparesort )
    ///constructing sorter
    CustomRankerPtr customRanker;
    Sorter* pSorter = NULL;
    try
    {
        prepare_sorter_customranker_(actionOperation,customRanker,pSorter);
    }
    catch (std::exception& e)
    {
        if(pSorter) delete pSorter;
        return false;
    }

    ///constructing collector
    boost::scoped_ptr<HitQueue> scoreItemQueue;

    int heapSize = topK + start;

    ///sortby
    if (pSorter)
        scoreItemQueue.reset(new PropertySortedHitQueue(pSorter, heapSize));
    else
        scoreItemQueue.reset(new ScoreSortedHitQueue(heapSize));

    STOP_PROFILER( preparesort )

    if (!pMultiPropertyIterator &&  !pWandDocIterator && !pFilterIdSet)
    {//SELECT * , and filter is null
        if (pSorter)
        {
            ///SELECT * ORDER BY
            unsigned maxDoc = documentManagerPtr_->getMaxDocId();
            if (maxDoc == 0)
                return false;
            boost::shared_ptr<BitVector> pDelFilter(indexManagerPtr_->getBTreeIndexer()->getFilter());
            AllDocumentIterator* pFilterIterator = NULL;
            if (pDelFilter)
            {
                pFilterIdSet.reset(new EWAHBoolArray<uint32_t>());
                pDelFilter->compressed(*pFilterIdSet);
                BitMapIterator* pBitmapIter = new BitMapIterator(pFilterIdSet);
                pFilterIterator = new AllDocumentIterator( pBitmapIter,maxDoc );
            }
            else
            {
                pFilterIterator = new AllDocumentIterator( maxDoc);
            }
            pDocIterator->add((DocumentIterator*) pFilterIterator);
        }
        else
        {
            return false;
        }
    }

    START_PROFILER( preparerank )

    ///prepare data for rankingmanager;
    DocumentFrequencyInProperties dfmap;
    CollectionTermFrequencyInProperties ctfmap;
    MaxTermFrequencyInProperties maxtfmap;

    if (distSearchInfo.effective_)
    {
        if (distSearchInfo.option_ == DistKeywordSearchInfo::OPTION_GATHER_INFO)
        {
            pDocIterator->df_cmtf(dfmap, ctfmap, maxtfmap);
            distSearchInfo.dfmap_ = dfmap;
            distSearchInfo.ctfmap_ = ctfmap;
            distSearchInfo.maxtfmap_ = maxtfmap;
            return true;
        }
        else if (distSearchInfo.option_ == DistKeywordSearchInfo::OPTION_CARRIED_INFO)
        {
            dfmap = distSearchInfo.dfmap_;
            ctfmap = distSearchInfo.ctfmap_;
            maxtfmap = distSearchInfo.maxtfmap_;
        }
    }
    else
    {
        pDocIterator->df_cmtf(dfmap, ctfmap, maxtfmap);
    }

    std::vector<RankQueryProperty> rankQueryProperties(indexPropertySize);

    post_prepare_ranker_(
            indexPropertyList,
            indexPropertySize,
            propertyTermInfoMap,
            dfmap,
            ctfmap,
            maxtfmap,
            readTermPosition,
            rankQueryProperties,
            propertyRankers);

    UpperBoundInProperties ubmap;
    if( isWandSearch && pWandDocIterator)
    {
        for (size_t i = 0; i < indexPropertySize; ++i )
        {
            const std::string& currentProperty = indexPropertyList[i];
            ID_FREQ_MAP_T& ub = ubmap[currentProperty];
            propertyRankers[i]->calculateTermUBs(rankQueryProperties[i], ub);
        }
        pWandDocIterator->set_ub( ubmap );
        pWandDocIterator->init_threshold(actionOperation.actionItem_.searchingMode_.threshold_);
    }

    STOP_PROFILER( preparerank )

    boost::scoped_ptr<faceted::GroupFilter> groupFilter;
    if (groupFilterBuilder_)
    {
        groupFilter.reset(
                groupFilterBuilder_->createFilter(actionOperation.actionItem_.groupParam_));
    }

    bool ret = false;

    try
    {
        ret = doSearch_(
            isWandSearch,
            actionOperation,
            docIdList,
            rankScoreList,
            customRankScoreList,
            totalCount,
            propertyRange,
            start,
            rankQueryProperties,
            propertyRankers,
            pSorter,
            customRanker,
            pMultiPropertyIterator,
            pWandDocIterator,
            pDocIterator.get(),
            groupFilter.get(),
            scoreItemQueue.get(),
            heapSize);

        if (groupFilter)
            groupFilter->getGroupRep(groupRep, attrRep);

        ///rerank is only used for pure ranking
        std::vector<std::pair<std::string, bool> >& sortPropertyList
        = actionOperation.actionItem_.sortPriorityList_;
        bool rerank = false;
        if (!pSorter) rerank = true;
        else if (sortPropertyList.size() == 1)
        {
            std::string fieldNameL = sortPropertyList[0].first;
            boost::to_lower(fieldNameL);
            if (fieldNameL == RANK_PROPERTY)
                rerank = true;
        }
        else
        {
            std::vector<std::pair<std::string, bool> >::iterator it =
                std::find_if(sortPropertyList.begin(),sortPropertyList.end(),hasRelevenceSort);
            if (it != sortPropertyList.end())
                rerank = true;
        }


        if (rerank && reranker_)
        {
            reranker_(docIdList,rankScoreList,actionOperation.actionItem_.env_.queryString_);
        }

        if (distSearchInfo.effective_ && pSorter)
        {
            getSortPropertyData_(pSorter, docIdList, distSearchInfo);
        }
    }
    catch (std::exception& e)
    {
        if(pSorter) delete pSorter;
        return false;
    }
    if(pSorter) delete pSorter;
    return ret;
}


bool SearchManager::doSearch_(
        bool isWandSearch,
        SearchKeywordOperation& actionOperation,
        std::vector<unsigned int>& docIdList,
        std::vector<float>& rankScoreList,
        std::vector<float>& customRankScoreList,
        std::size_t& totalCount,
        sf1r::PropertyRange& propertyRange,
        uint32_t start,
        std::vector<RankQueryProperty>& rankQueryProperties,
        std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers,
        Sorter* pSorter,
        CustomRankerPtr customRanker,
        MultiPropertyScorer* pMultiPropertyIterator,
        WANDDocumentIterator* pWandDocIterator,
        CombinedDocumentIterator* pDocIterator,
        faceted::GroupFilter* groupFilter,
        HitQueue* scoreItemQueue,
        int heapSize)
{
CREATE_PROFILER( computerankscore, "SearchManager", "doSearch_: overall time for scoring a doc");
CREATE_PROFILER( inserttoqueue, "SearchManager", "doSearch_: overall time for inserting to result queue");
CREATE_PROFILER( computecustomrankscore, "SearchManager", "doSearch_: overall time for scoring customized score for a doc");
    totalCount = 0;
    const std::string& rangePropertyName = actionOperation.actionItem_.rangePropertyName_;
    boost::scoped_ptr<NumericPropertyTable> rangePropertyTable;
    float lowValue = (std::numeric_limits<float>::max) ();
    float highValue = - lowValue;

    if (!rangePropertyName.empty())
    {
        typedef boost::unordered_map<std::string, PropertyConfig>::const_iterator iterator;
        rangePropertyTable.reset(createPropertyTable(rangePropertyName));
    }
    bool requireScorer = false;
    if ( (pMultiPropertyIterator || pWandDocIterator) && pSorter ) requireScorer = pSorter->requireScorer();
    while (pDocIterator->next())
    {
        if (groupFilter && !groupFilter->test(pDocIterator->doc()))
            continue;

        if (rangePropertyTable)
        {
            float docPropertyValue = 0;
            if (rangePropertyTable->convertPropertyValue(pDocIterator->doc(), docPropertyValue))
            {
                if (docPropertyValue < lowValue)
                {
                    lowValue = docPropertyValue;
                }

               if (docPropertyValue > highValue)
               {
                    highValue = docPropertyValue;
               }
            }
        }

        ScoreDoc scoreItem(pDocIterator->doc());
        START_PROFILER( computerankscore )
        ++totalCount;
        if( !isWandSearch )
        {
            scoreItem.score = requireScorer?
                              pMultiPropertyIterator->score(
                                  rankQueryProperties,
                                  propertyRankers) : 1.0;
        }
        else
        {
            scoreItem.score = requireScorer?
                              pWandDocIterator->score(
                                  rankQueryProperties,
                                  propertyRankers) : 1.0;
        }
        STOP_PROFILER( computerankscore )

        START_PROFILER( computecustomrankscore )
        if (customRanker.get())
        {
            scoreItem.custom_score = customRanker->evaluate(scoreItem.docId);
        }
        STOP_PROFILER( computecustomrankscore )

        START_PROFILER( inserttoqueue )
        scoreItemQueue->insert(scoreItem);
        if( isWandSearch && scoreItemQueue->size() == (unsigned)heapSize )
            pWandDocIterator->set_threshold((scoreItemQueue->top()).score);
        STOP_PROFILER( inserttoqueue )

    }

    if (rangePropertyTable && lowValue <= highValue)
    {
        propertyRange.highValue_ = highValue;
        propertyRange.lowValue_ = lowValue;
    }

    size_t count = start > 0 ? (scoreItemQueue->size() - start) : scoreItemQueue->size();
    docIdList.resize(count);
    rankScoreList.resize(count);
    if (customRanker.get())
    {
        customRankScoreList.resize(count);
    }

    ///Attention
    ///Given default lessthan() for prorityqueue
    //pop will get the elements of priorityqueue with lowest score
    ///So we need to reverse the output sequence
    float min = std::numeric_limits<float>::max();
    float max = - min;
    for (size_t i = 0; i < count; ++i)
    {
        ScoreDoc pScoreItem = scoreItemQueue->pop();
        docIdList[count - i - 1] = pScoreItem.docId;
        rankScoreList[count - i - 1] = pScoreItem.score;
        if (customRanker.get())
        {
            // should not be normalized
            customRankScoreList[count - i - 1] = pScoreItem.custom_score;
        }

        if (pScoreItem.score < min)
        {
            min = pScoreItem.score;
        }
        // cannot use "else if" because the first score is between min
        // and max
        if (pScoreItem.score > max)
        {
            max = pScoreItem.score;
        }
        /*
        DocumentItem* item = pScoreItem->pItem;
        //izenelib::util::swapBack(commonSet, *item);
        commonSet.push_front(DocumentItem());
        using std::swap;
        swap(commonSet.front(), *item);
        */
        //delete pScoreItem;
    }

#if 0
    // normalize rank score to [0, 1]
    float range = max - min;
    if (range != 0)
    {
        for (std::vector<float>::iterator it = rankScoreList.begin(),
                itEnd = rankScoreList.end();
                it != itEnd; ++it)
        {
            *it -= min;
            *it /= range;
        }
    }
    else
    {
        std::fill(rankScoreList.begin(), rankScoreList.end(), 1.0F);
    }
#endif
    return count > 0? true:false;
}


void SearchManager::post_prepare_ranker_(
        const std::vector<std::string>& indexPropertyList,
        unsigned indexPropertySize,
        const property_term_info_map& propertyTermInfoMap,
        DocumentFrequencyInProperties& dfmap,
        CollectionTermFrequencyInProperties& ctfmap,
        MaxTermFrequencyInProperties& maxtfmap,
        bool readTermPosition,
        std::vector<RankQueryProperty>& rankQueryProperties,
        std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers)
{
    static const PropertyTermInfo emptyPropertyTermInfo;
    for (unsigned i = 0; i < indexPropertySize; ++i)
    {
        const std::string& currentProperty = indexPropertyList[i];

        rankQueryProperties[i].setNumDocs(
                indexManagerPtr_->numDocs()
        );

        rankQueryProperties[i].setTotalPropertyLength(
                documentManagerPtr_->getTotalPropertyLength(currentProperty)
        );

        const PropertyTermInfo::id_uint_list_map_t& termPositionsMap =
            izenelib::util::getOr(
                    propertyTermInfoMap,
                    currentProperty,
                    emptyPropertyTermInfo
            ).getTermIdPositionMap();

        typedef PropertyTermInfo::id_uint_list_map_t::const_iterator
        term_id_position_iterator;
        unsigned queryLength = 0;
        unsigned index = 0;
        for (term_id_position_iterator termIt = termPositionsMap.begin();
                termIt != termPositionsMap.end(); ++termIt, ++index)
        {
            rankQueryProperties[i].addTerm(termIt->first);
            rankQueryProperties[i].setTotalTermFreq(
                    ctfmap[currentProperty][termIt->first]
            );
            rankQueryProperties[i].setDocumentFreq(
                    dfmap[currentProperty][termIt->first]
            );

            rankQueryProperties[i].setMaxTermFreq(
                    maxtfmap[currentProperty][termIt->first]
            );

            queryLength += termIt->second.size();
            if (readTermPosition)
            {
                typedef PropertyTermInfo::id_uint_list_map_t::mapped_type
                uint_list_map_t;
                typedef uint_list_map_t::const_iterator uint_list_iterator;
                for (uint_list_iterator posIt = termIt->second.begin();
                        posIt != termIt->second.end(); ++posIt)
                {
                    rankQueryProperties[i].pushPosition(*posIt);
                }
            }
            else
            {
                rankQueryProperties[i].setTermFreq(termIt->second.size());
            }
        }

        rankQueryProperties[i].setQueryLength(queryLength);
    }

    for (size_t i = 0; i < indexPropertySize; ++i )
    {
        propertyRankers[i]->setupStats(rankQueryProperties[i]);
    }
}

void SearchManager::prepare_sorter_customranker_(
    SearchKeywordOperation& actionOperation,
    CustomRankerPtr& customRanker,
    Sorter* &pSorter)
{
    std::vector<std::pair<std::string, bool> >& sortPropertyList
        = actionOperation.actionItem_.sortPriorityList_;
    if (!sortPropertyList.empty())
    {
        std::vector<std::pair<std::string, bool> >::iterator iter = sortPropertyList.begin();
        for (; iter != sortPropertyList.end(); ++iter)
        {
            std::string fieldNameL = iter->first;
            boost::to_lower(fieldNameL);
            // sort by custom ranking
            if (fieldNameL == CUSTOM_RANK_PROPERTY)
            {
                // prepare custom ranker data, custom score will be evaluated later as rank score
                customRanker = actionOperation.actionItem_.customRanker_;
                if (!customRanker)
                    customRanker = buildCustomRanker_(actionOperation.actionItem_);
                if (!customRanker->setPropertyData(pSorterCache_))
                {
                    LOG(ERROR) << customRanker->getErrorInfo() ;
                    continue;
                }
                //customRanker->printESTree();

                if (!pSorter) pSorter = new Sorter(pSorterCache_);
                SortProperty* pSortProperty = new SortProperty(
                        "CUSTOM_RANK",
                        CUSTOM_RANKING_PROPERTY_TYPE,
                        SortProperty::CUSTOM,
                        iter->second);
                pSorter->addSortProperty(pSortProperty);
                continue;
            }
            // sort by rank
            if (fieldNameL == RANK_PROPERTY)
            {
                if (!pSorter) pSorter = new Sorter(pSorterCache_);
                SortProperty* pSortProperty = new SortProperty(
                        "RANK",
                        UNKNOWN_DATA_PROPERTY_TYPE,
                        SortProperty::SCORE,
                        iter->second);
                pSorter->addSortProperty(pSortProperty);
                continue;
            }
            // sort by date
            if (fieldNameL == DATE_PROPERTY)
            {
                if (!pSorter) pSorter = new Sorter(pSorterCache_);
                SortProperty* pSortProperty = new SortProperty(
                        iter->first,
                        INT_PROPERTY_TYPE,
                        iter->second);
                pSorter->addSortProperty(pSortProperty);
                continue;
            }
            // sort by ctr (click through rate)
            if (fieldNameL == CTR_PROPERTY)
            {
                if (miningManagerPtr_.expired())
                {
                    DLOG(ERROR)<<"Skipped CTR sort property: Mining Manager was not initialized";
                    continue;
                }
                boost::shared_ptr<MiningManager> resourceMiningManagerPtr = miningManagerPtr_.lock();
                boost::shared_ptr<faceted::CTRManager> ctrManangerPtr
                = resourceMiningManagerPtr->GetCtrManager();
                if (!ctrManangerPtr)
                {
                    DLOG(ERROR)<<"Skipped CTR sort property: CTR Manager was not initialized";
                    continue;
                }

                pSorterCache_->setCtrManager(ctrManangerPtr.get());
                if (!pSorter) pSorter = new Sorter(pSorterCache_);

                SortProperty* pSortProperty = new SortProperty(
                        iter->first,
                        UNSIGNED_INT_PROPERTY_TYPE,
                        SortProperty::CTR,
                        iter->second);
                pSorter->addSortProperty(pSortProperty);
                continue;
            }

            // sort by arbitrary property
            boost::unordered_map<std::string, PropertyConfig>::iterator it
                = schemaMap_.find(iter->first);
            if (it == schemaMap_.end())
                continue;

            PropertyConfig& propertyConfig = it->second;
            if (!propertyConfig.isIndex() || propertyConfig.isAnalyzed())
                continue;

            PropertyDataType propertyType = propertyConfig.getType();
            switch (propertyType)
            {
            case INT_PROPERTY_TYPE:
            case FLOAT_PROPERTY_TYPE:
            case NOMINAL_PROPERTY_TYPE:
            case UNSIGNED_INT_PROPERTY_TYPE:
            case DOUBLE_PROPERTY_TYPE:
            case STRING_PROPERTY_TYPE:
            {
                if (!pSorter) pSorter = new Sorter(pSorterCache_);
                SortProperty* pSortProperty = new SortProperty(
                    iter->first,
                    propertyType,
                    iter->second);
                pSorter->addSortProperty(pSortProperty);
                break;
            }
            default:
                DLOG(ERROR) << "Sort by properties other than int, float, double type"; // TODO : Log
                break;
            }
        }
    }
}

bool SearchManager::getPropertyTypeByName_(
        const std::string& name,
        PropertyDataType& type) const
{
    boost::unordered_map<std::string, PropertyConfig>::const_iterator it
        = schemaMap_.find(name);

    if (it != schemaMap_.end())
    {
        type = it->second.getType();
        return true;
    }

    return false;
}

CustomRankerPtr
SearchManager::buildCustomRanker_(KeywordSearchActionItem& actionItem)
{
    CustomRankerPtr customRanker(new CustomRanker());

    customRanker->getConstParamMap() = actionItem.paramConstValueMap_;
    customRanker->getPropertyParamMap() = actionItem.paramPropertyValueMap_;

    customRanker->parse(actionItem.strExp_);

    std::map<std::string, PropertyDataType>& propertyDataTypeMap
        = customRanker->getPropertyDataTypeMap();
    std::map<std::string, PropertyDataType>::iterator iter
        = propertyDataTypeMap.begin();
    for (; iter != propertyDataTypeMap.end(); iter++)
    {
        getPropertyTypeByName_(iter->first, iter->second);
    }

    return customRanker;
}

void SearchManager::getSortPropertyData_(
        Sorter* pSorter,
        std::vector<unsigned int>& docIdList,
        DistKeywordSearchInfo& distSearchInfo)
{
    if (!pSorter)
        return;

    size_t docNum = docIdList.size();
    std::list<SortProperty*>& sortProperties = pSorter->sortProperties_;
    std::list<SortProperty*>::iterator iter;
    SortProperty* pSortProperty;

    for (iter = sortProperties.begin(); iter != sortProperties.end(); ++iter)
    {
        pSortProperty = *iter;
        std::string SortPropertyName = pSortProperty->getProperty();
        distSearchInfo.sortPropertyList_.push_back(
                std::make_pair(SortPropertyName, pSortProperty->isReverse()));

        if (SortPropertyName == "CUSTOM_RANK" || SortPropertyName == "RANK")
            continue;

        boost::shared_ptr<PropertyData> propData = getPropertyData_(SortPropertyName);
        if (!propData)
            continue;

        void* data = propData->data_;
        size_t size = propData->size_;
        switch (propData->type_)
        {
        case INT_PROPERTY_TYPE:
            {
                std::vector<int64_t> dataList(docNum);
                for (size_t i = 0; i < docNum; i++)
                {
                    if (docIdList[i] >= size) dataList[i] = 0;
                    else dataList[i] = ((int64_t*) data)[docIdList[i]];
                }
                distSearchInfo.sortPropertyIntDataList_.push_back(
                        std::make_pair(SortPropertyName, dataList));
            }
            break;
        case UNSIGNED_INT_PROPERTY_TYPE:
            {
                std::vector<uint64_t> dataList(docNum);
                for (size_t i = 0; i < docNum; i++)
                {
                    if (docIdList[i] >= size) dataList[i] = 0;
                    else dataList[i] = ((uint64_t*) data)[docIdList[i]];
                }
                distSearchInfo.sortPropertyUIntDataList_.push_back(std::make_pair(SortPropertyName, dataList));
            }
            break;
        case FLOAT_PROPERTY_TYPE:
            {
                std::vector<float> dataList(docNum);
                for (size_t i = 0; i < docNum; i++)
                {
                    if (docIdList[i] >= size) dataList[i] = 0.0;
                    else dataList[i] = ((float*) data)[docIdList[i]];
                }
                distSearchInfo.sortPropertyFloatDataList_.push_back(std::make_pair(SortPropertyName, dataList));
            }
            break;
        case DOUBLE_PROPERTY_TYPE:
            {
                std::vector<float> dataList(docNum);
                for (size_t i = 0; i < docNum; i++)
                {
                    if (docIdList[i] >= size) dataList[i] = 0.0;
                    else dataList[i] = (float)((double*) data)[docIdList[i]];
                }
                distSearchInfo.sortPropertyFloatDataList_.push_back(std::make_pair(SortPropertyName, dataList));
            }
            break;
        default:
            break;
        }
    }

}

boost::shared_ptr<PropertyData>
SearchManager::getPropertyData_(const std::string& name)
{
    boost::shared_ptr<PropertyData> propData;
    PropertyDataType type = UNKNOWN_DATA_PROPERTY_TYPE;

    if (getPropertyTypeByName_(name, type))
    {
        propData = pSorterCache_->getSortPropertyData(name, type);
    }

    return propData;
}

void SearchManager::printDFCTF_(
    DocumentFrequencyInProperties& dfmap,
    CollectionTermFrequencyInProperties ctfmap)
{
    LOG(INFO)  << "\n[Index terms info for Query]";
    DocumentFrequencyInProperties::iterator dfiter;
    for (dfiter = dfmap.begin(); dfiter != dfmap.end(); dfiter++)
    {
        LOG(INFO) << "property: " << dfiter->first;
        ID_FREQ_UNORDERED_MAP_T::iterator iter_;
        for (iter_ = dfiter->second.begin(); iter_ != dfiter->second.end(); iter_++)
        {
            LOG(INFO)  << "termid: " << iter_->first << " DF: " << iter_->second;
        }
    }
    CollectionTermFrequencyInProperties::iterator ctfiter;
    for (ctfiter = ctfmap.begin(); ctfiter != ctfmap.end(); ctfiter++)
    {
        LOG(INFO) << "property: " << ctfiter->first;
        ID_FREQ_UNORDERED_MAP_T::iterator iter_;
        for (iter_ = ctfiter->second.begin(); iter_ != ctfiter->second.end(); iter_++)
        {
            LOG(INFO) << "termid: " << iter_->first << " CTF: " << iter_->second;
        }
    }
}

} // namespace sf1r
