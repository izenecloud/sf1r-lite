#include <common/SFLogger.h>

#include <bundles/index/IndexBundleConfiguration.h>
#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>
#include <query-manager/QueryIdentity.h>
#include <query-manager/ActionItem.h>
#include <ranking-manager/RankingManager.h>
#include <ranking-manager/RankQueryProperty.h>
#include <mining-manager/faceted-submanager/GroupFilterBuilder.h>
#include <mining-manager/faceted-submanager/GroupFilter.h>
#include <mining-manager/faceted-submanager/ontology_rep.h>
#include <common/SFLogger.h>

#include "SearchManager.h"
#include "CustomRanker.h"
#include "SearchCache.h"
#include "QueryBuilder.h"
#include "Sorter.h"
#include "HitQueue.h"
#include "FilterDocumentIterator.h"

#include <util/swap.h>
#include <util/get.h>
#include <net/aggregator/MasterServerBase.h>

#include <boost/algorithm/string.hpp>

#include <fstream>
#include <algorithm>

using namespace net::aggregator;

namespace sf1r
{

const char* RANK_PROPERTY = "_rank";
const char* DATE_PROPERTY = "date";

bool hasRelevenceSort(const std::pair<std::string , bool>& element)
{
    std::string fieldNameL = element.first;
    boost::to_lower(fieldNameL);
    if (fieldNameL == RANK_PROPERTY)
        return true;
    return false;
}

bool action_rerankable(SearchKeywordOperation& actionOperation)
{
    bool rerank = false;
    if (actionOperation.actionItem_.env_.taxonomyLabel_.empty() &&
            actionOperation.actionItem_.env_.nameEntityItem_.empty() &&
            actionOperation.actionItem_.groupParam_.groupLabels_.empty() &&
            actionOperation.actionItem_.groupParam_.attrLabels_.empty())
    {
        std::vector<std::pair<std::string , bool> >& sortPropertyList = actionOperation.actionItem_.sortPriorityList_;
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
            std::vector<std::pair<std::string , bool> >::iterator it =
                std::find_if(sortPropertyList.begin(),sortPropertyList.end(),hasRelevenceSort);
            if (it != sortPropertyList.end())
                rerank = true;
        }
    }
    return rerank;
}

SearchManager::SearchManager(
    std::set<PropertyConfig, PropertyComp> schema,
    const boost::shared_ptr<IDManager>& idManager,
    const boost::shared_ptr<DocumentManager>& documentManager,
    const boost::shared_ptr<IndexManager>& indexManager,
    const boost::shared_ptr<RankingManager>& rankingManager,
    IndexBundleConfiguration* config
)
        : schemaMap_()
        , indexManagerPtr_(indexManager)
        , documentManagerPtr_(documentManager)
        , rankingManagerPtr_(rankingManager)
        , queryBuilder_()
        , reranker_(0)
{
    collectionName_ = config->collectionName_;
    for (std::set<PropertyConfig, PropertyComp>::iterator iter = schema.begin(); iter != schema.end(); ++iter)
        schemaMap_[iter->getName()] = *iter;

    pSorterCache_ = new SortPropertyCache(indexManagerPtr_.get(), config);
    queryBuilder_.reset(new QueryBuilder(indexManager,documentManager,idManager,schemaMap_, config->filterCacheNum_));
    cache_.reset(new SearchCache(config->searchCacheNum_));
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
    if ( !rType )
    {
        pSorterCache_->setDirty(true);
    }
    else
    {
        pSorterCache_->updateSortData(id, rTypeFieldValue);
    }

    cache_->clear();
    {
        NotifyMSG msg;
        msg.identity = collectionName_;
        msg.method = "clear_search_cache";
        MasterNotifierSingleton::get()->notify(msg);
    }

    queryBuilder_->reset_cache();
}

void SearchManager::reset_all_property_cache()
{
    pSorterCache_->setDirty(true);
    cache_->clear();
    {
        NotifyMSG msg;
        msg.identity = collectionName_;
        msg.method = "clear_search_cache";
        MasterNotifierSingleton::get()->notify(msg);
    }

    queryBuilder_->reset_cache();
}

/// @brief change working dir by setting new underlying componenets
void SearchManager::chdir(
    const boost::shared_ptr<IDManager>& idManager,
    const boost::shared_ptr<DocumentManager>& documentManager,
    const boost::shared_ptr<IndexManager>& indexManager,
    IndexBundleConfiguration* config)
{
    indexManagerPtr_ = indexManager;
    documentManagerPtr_ = documentManager;

    // init query builder
    queryBuilder_.reset(new QueryBuilder(indexManager, documentManager, idManager, schemaMap_, config->filterCacheNum_));

    // clear cache
    delete pSorterCache_;
    pSorterCache_ = new SortPropertyCache(indexManagerPtr_.get(), config);
    cache_.reset(new SearchCache(config->searchCacheNum_));
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
    int topK,
    int start)
{
    CREATE_PROFILER ( cacheoverhead, "SearchManager", "cache overhead: overhead for caching in searchmanager");

    START_PROFILER ( cacheoverhead )

    izenelib::ir::indexmanager::IndexManagerConfig* pIndexManagerConfig = indexManagerPtr_->getIndexManagerConfig();
    if ( pIndexManagerConfig->indexStrategy_.indexLevel_ == izenelib::ir::indexmanager::DOCLEVEL &&
            actionOperation.actionItem_.rankingType_ == RankingType::PLM )
    {
        actionOperation.actionItem_.rankingType_ = RankingType::BM25;
    }

    QueryIdentity identity;
    makeQueryIdentity(identity, actionOperation.actionItem_, distSearchInfo.actionType_, start);

    if (cache_->get(
                identity,
                rankScoreList,
                customRankScoreList,
                docIdList,
                totalCount,
                groupRep,
                attrRep,
                propertyRange,
                distSearchInfo))
    {
        STOP_PROFILER ( cacheoverhead )
        // the cached search results require to be reranked
        if (reranker_ &&
                customRankScoreList.empty() &&
                action_rerankable(actionOperation))
            reranker_(docIdList,rankScoreList,actionOperation.actionItem_.env_.queryString_);

        return true;
    }

    STOP_PROFILER ( cacheoverhead )

    // cache miss
    if (doSearch_(
                actionOperation,
                docIdList,
                rankScoreList,
                customRankScoreList,
                totalCount,
                groupRep,
                attrRep,
                propertyRange,
                distSearchInfo,
                topK,
                start))
    {
        START_PROFILER ( cacheoverhead )
        cache_->set(
            identity,
            rankScoreList,
            customRankScoreList,
            docIdList,
            totalCount,
            groupRep,
            attrRep,
            propertyRange,
            distSearchInfo);
        STOP_PROFILER ( cacheoverhead )
        return true;
    }

    return false;
}

bool SearchManager::doSearch_(
    SearchKeywordOperation& actionOperation,
    std::vector<unsigned int>& docIdList,
    std::vector<float>& rankScoreList,
    std::vector<float>& customRankScoreList,
    std::size_t& totalCount,
    faceted::GroupRep& groupRep,
    faceted::OntologyRep& attrRep,
    sf1r::PropertyRange& propertyRange,
    DistKeywordSearchInfo& distSearchInfo,
    int topK,
    int start)
{
    CREATE_PROFILER ( dociterating, "SearchManager", "doSearch_: doc iterating");
    CREATE_PROFILER ( preparedociter, "SearchManager", "doSearch_: SearchManager_search : build doc iterator");
    CREATE_PROFILER ( preparerank, "SearchManager", "doSearch_: prepare ranker");
    CREATE_PROFILER ( preparesort, "SearchManager", "doSearch_: prepare sort");
    CREATE_PROFILER ( computerankscore, "SearchManager", "doSearch_: overall time for scoring a doc");
    CREATE_PROFILER ( computecustomrankscore, "SearchManager", "doSearch_: overall time for scoring customized score for a doc");

    unsigned int collectionId = 1;
    std::vector<std::string> indexPropertyList( actionOperation.actionItem_.searchPropertyList_ );
#if PREFETCH_TERMID
    std::stable_sort (indexPropertyList.begin(), indexPropertyList.end());
#endif
    typedef std::vector<std::string>::size_type property_size_type;
    property_size_type indexPropertySize = indexPropertyList.size();
    std::vector<propertyid_t> indexPropertyIdList(indexPropertySize);
    std::transform(
        indexPropertyList.begin(),
        indexPropertyList.end(),
        indexPropertyIdList.begin(),
        boost::bind(&SearchManager::getPropertyIdByName_, this, _1)
    );

    if ( actionOperation.noError() == false )
        return false;

    START_PROFILER ( preparedociter )
    sf1r::TextRankingType& pTextRankingType = actionOperation.actionItem_.rankingType_;

    // references for property term info
    typedef std::map<std::string, PropertyTermInfo> property_term_info_map;
    const property_term_info_map& propertyTermInfoMap =
        actionOperation.getPropertyTermInfoMap();
    // use empty object for not found property
    const PropertyTermInfo emptyPropertyTermInfo;

    // build term index maps
    std::vector<std::map<termid_t, unsigned> > termIndexMaps(indexPropertySize);
    typedef std::vector<std::string>::const_iterator property_list_iterator;
    for (std::vector<std::string>::size_type i = 0;
            i != indexPropertyList.size(); ++i)
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

    boost::scoped_ptr<ANDDocumentIterator> pDocIterator;
    MultiPropertyScorer* pMultiPropertyIterator = NULL;
    std::vector<QueryFiltering::FilteringType>& filtingList
    = actionOperation.actionItem_.filteringList_;
    boost::shared_ptr<EWAHBoolArray<uint32_t> > pFilterIdSet;
    try
    {
        if (!filtingList.empty())
            queryBuilder_->prepare_filter(filtingList, pFilterIdSet);

        pMultiPropertyIterator = queryBuilder_->prepare_dociterator(
                                     actionOperation,
                                     collectionId,
                                     propertyWeightMap_,
                                     indexPropertyList,
                                     indexPropertyIdList,
                                     readTermPosition,
                                     termIndexMaps
                                 );

        if (pFilterIdSet)
        {
            BitMapIterator* pBitmapIter = new BitMapIterator(pFilterIdSet->bit_iterator());
            FilterDocumentIterator* pFilterIterator = new FilterDocumentIterator( pBitmapIter );
            pDocIterator.reset(new ANDDocumentIterator());
            pDocIterator->add((DocumentIterator*)pFilterIterator);
        }

        if (pMultiPropertyIterator)
        {
            if (!pDocIterator)
            {
                pDocIterator.reset(new ANDDocumentIterator());
            }
            pDocIterator->add((DocumentIterator*)pMultiPropertyIterator);
        }

        //if(!pDocIterator)
        //  return false;
    }
    catch (std::exception& e)
    {
        return false;
    }
    STOP_PROFILER ( preparedociter )

    START_PROFILER ( preparesort )
    ///constructing sorter
    CustomRankerPtr customRanker;
    boost::scoped_ptr<Sorter> pSorter;
    try
    {
        std::vector<std::pair<std::string , bool> >& sortPropertyList = actionOperation.actionItem_.sortPriorityList_;
        if (!sortPropertyList.empty())
        {
            for (std::vector<std::pair<std::string , bool> >::iterator iter = sortPropertyList.begin(); iter != sortPropertyList.end(); ++iter)
            {
                std::string fieldNameL = iter->first;
                boost::to_lower(fieldNameL);
                // sort by custom ranking
                if (fieldNameL == "custom_rank")
                {
                    // prepare custom ranker
                    customRanker = actionOperation.actionItem_.customRanker_;
                    if (!customRanker)
                        customRanker = buildCustomRanker_(actionOperation.actionItem_);
                    //customRanker->printESTree(); //test
                    if (!customRanker->setPropertyData(pSorterCache_))
                    {
                        // error info
                        LOG(ERROR) << customRanker->getErrorInfo() << endl;
                        return false;
                    }
                    customRanker->printESTree(); //test

                    if (!pSorter) pSorter.reset(new Sorter(pSorterCache_));
                    SortProperty* pSortProperty = new SortProperty("CUSTOM_RANK", CUSTOM_RANKING_PROPERTY_TYPE, SortProperty::CUSTOM, iter->second);
                    pSorter->addSortProperty(pSortProperty);
                    continue;
                }
                // sort by rank
                if (fieldNameL == RANK_PROPERTY)
                {
                    if (!pSorter) pSorter.reset(new Sorter(pSorterCache_));
                    SortProperty* pSortProperty = new SortProperty("RANK", UNKNOWN_DATA_PROPERTY_TYPE, SortProperty::SCORE, iter->second);
                    pSorter->addSortProperty(pSortProperty);
                    continue;
                }
                // sort by date
                if (fieldNameL == DATE_PROPERTY)
                {
                    if (!pSorter) pSorter.reset(new Sorter(pSorterCache_));

                    SortProperty* pSortProperty = new SortProperty(iter->first, INT_PROPERTY_TYPE, iter->second);
                    pSorter->addSortProperty(pSortProperty);
                    continue;
                }

                // sort by arbitrary property
                boost::unordered_map<std::string, PropertyConfig>::iterator it = schemaMap_.find(iter->first);
                //std::cout << "sort by " << iter->first << std::endl;
                if (it == schemaMap_.end())
                    continue;

                PropertyConfig& propertyConfig = it->second;
                if (!propertyConfig.isIndex()||propertyConfig.isAnalyzed())
                    continue;

                PropertyDataType propertyType = propertyConfig.getType();
                switch (propertyType)
                {
                case INT_PROPERTY_TYPE:
                case FLOAT_PROPERTY_TYPE:
                case NOMINAL_PROPERTY_TYPE:
                case UNSIGNED_INT_PROPERTY_TYPE:
                case DOUBLE_PROPERTY_TYPE:
                {
                    if (!pSorter) pSorter.reset(new Sorter(pSorterCache_));
                    SortProperty* pSortProperty = new SortProperty(iter->first, propertyType, iter->second);
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
    catch (std::exception& e)
    {
        return false;
    }

    ///constructing collector
    boost::scoped_ptr<HitQueue> scoreItemQueue;

    int heapSize = topK + start;

    ///sortby
    if (pSorter)
        scoreItemQueue.reset(new PropertySortedHitQueue(pSorter.get(), heapSize));
    else
        scoreItemQueue.reset(new ScoreSortedHitQueue(heapSize));

    STOP_PROFILER ( preparesort )

    if (!pDocIterator)
    {
        if (pSorter)
        {
            pDocIterator.reset(new ANDDocumentIterator());
            prepareDocIterWithOnlyOrderby_(pDocIterator.get(), pFilterIdSet);
        }
        else
        {
            return false;
        }
    }


    START_PROFILER ( preparerank )

    ///prepare data for rankingmanager;
    DocumentFrequencyInProperties dfmap;
    CollectionTermFrequencyInProperties ctfmap;

    if (distSearchInfo.actionType_ == DistKeywordSearchInfo::ACTION_FETCH)
    {
        pDocIterator->df_ctf(dfmap, ctfmap);

        distSearchInfo.dfmap_ = dfmap;
        distSearchInfo.ctfmap_ = ctfmap;
        return true;
    }
    else if (distSearchInfo.actionType_ == DistKeywordSearchInfo::ACTION_SEND)
    {
        dfmap = distSearchInfo.dfmap_;
        ctfmap = distSearchInfo.ctfmap_;
    }
    else
    {
        pDocIterator->df_ctf(dfmap, ctfmap);
    }


    vector<RankQueryProperty> rankQueryProperties(indexPropertySize);

    for (property_size_type i = 0; i < indexPropertySize; ++i)
    {
        const std::string& currentProperty = indexPropertyList[i];

        rankQueryProperties[i].setNumDocs(
            indexManagerPtr_->getIndexReader()->numDocs()
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


    STOP_PROFILER ( preparerank )

    boost::scoped_ptr<faceted::GroupFilter> groupFilter;
    if (groupFilterBuilder_)
    {
        groupFilter.reset(groupFilterBuilder_->createFilter(actionOperation.actionItem_.groupParam_));
    }

    START_PROFILER ( dociterating )
    try
    {
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

        while (pDocIterator->next())
        {
            if (groupFilter && !groupFilter->test(pDocIterator->doc()))
                continue;

            if (rangePropertyTable)
            {
                float docPropertyValue = 0;
                if (rangePropertyTable->convertPropertyValue(pDocIterator->doc(), docPropertyValue))
                {
                    if ( docPropertyValue < lowValue )
                    {
                        lowValue = docPropertyValue;
                    }

                    if (docPropertyValue > highValue)
                    {
                        highValue = docPropertyValue;
                    }
                }
            }

            STOP_PROFILER ( dociterating )
            ScoreDoc scoreItem(pDocIterator->doc());
            START_PROFILER ( computerankscore )
            ++totalCount;
            if (pMultiPropertyIterator)
                scoreItem.score = pMultiPropertyIterator->score(
                                      rankQueryProperties,
                                      propertyRankers
                                  );
            else
                scoreItem.score = 1;
            STOP_PROFILER ( computerankscore )

            START_PROFILER ( computecustomrankscore )
            if (customRanker.get())
            {
                scoreItem.custom_score = customRanker->evaluate(scoreItem.docId);
            }
            STOP_PROFILER ( computecustomrankscore )

            scoreItemQueue->insert(scoreItem);
            START_PROFILER ( dociterating )
        }
        STOP_PROFILER ( dociterating )

        if (groupFilter)
            groupFilter->getGroupRep(groupRep, attrRep);

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
        ///pop will get the elements of priorityqueue with lowest score
        ///So we need to reverse the output sequence
        float min = (std::numeric_limits<float>::max) ();
        float max = - min;
        for (size_t i = 0; i < count; ++i)
        {
            ScoreDoc pScoreItem = scoreItemQueue->pop();
            docIdList[count - i - 1] = pScoreItem.docId;
            rankScoreList[count - i - 1] = pScoreItem.score;
            if (customRanker.get())
            {
                customRankScoreList[count - i - 1] = pScoreItem.custom_score; // should not be normalized
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

        ///rerank is only used for pure ranking
        std::vector<std::pair<std::string , bool> >& sortPropertyList = actionOperation.actionItem_.sortPriorityList_;
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
            std::vector<std::pair<std::string , bool> >::iterator it =
                std::find_if(sortPropertyList.begin(),sortPropertyList.end(),hasRelevenceSort);
            if (it != sortPropertyList.end())
                rerank = true;
        }


        if (rerank && reranker_)
        {
            reranker_(docIdList,rankScoreList,actionOperation.actionItem_.env_.queryString_);
        }

        if (pSorter)
        {
            getSortPropertyData_(pSorter.get(), docIdList, distSearchInfo);
        }
    }
    catch (std::exception& e)
    {
        return false;
    }

    return true;
}

propertyid_t SearchManager::getPropertyIdByName_(const std::string& name) const
{
    typedef boost::unordered_map<std::string, PropertyConfig>::const_iterator
    iterator;
    iterator found = schemaMap_.find(name);
    if (found != schemaMap_.end())
    {
        return found->second.getPropertyId();
    }
    else
    {
        return 0;
    }
}

void SearchManager::prepareDocIterWithOnlyOrderby_(
    ANDDocumentIterator* pDocIterator,
    boost::shared_ptr<EWAHBoolArray<uint32_t> >& pFilterIdSet)
{
    unsigned int bitsNum = documentManagerPtr_->getMaxDocId() + 1;
    unsigned int wordsNum = bitsNum/(sizeof(uint32_t) * 8);

    pFilterIdSet.reset(new EWAHBoolArray<uint32_t>());
    for (unsigned num = 1; num < sizeof(uint32_t) * 8; num++)
        pFilterIdSet->set(num);
    pFilterIdSet->addStreamOfEmptyWords(true, wordsNum - 1);
    for (unsigned int num = wordsNum * (sizeof(uint32_t) * 8); num <= bitsNum - 1; num++)
        pFilterIdSet->set(num);

    BitMapIterator* pBitmapIter = new BitMapIterator(pFilterIdSet->bit_iterator());
    FilterDocumentIterator* pFilterIterator = new FilterDocumentIterator( pBitmapIter );
    pDocIterator->add((DocumentIterator*)pFilterIterator);
}

bool SearchManager::getPropertyTypeByName_(
    const std::string& name,
    PropertyDataType& type) const
{
    boost::unordered_map<std::string, PropertyConfig>::const_iterator it = schemaMap_.find(name);

    if (it != schemaMap_.end())
    {
        type = it->second.getType();
        return true;
    }

    return false;
}

CustomRankerPtr
SearchManager::buildCustomRanker_(
    KeywordSearchActionItem& actionItem)
{
    CustomRankerPtr customRanker(new CustomRanker());

    customRanker->getConstParamMap() = actionItem.paramConstValueMap_;
    customRanker->getPropertyParamMap() = actionItem.paramPropertyValueMap_;

    customRanker->parse(actionItem.strExp_);

    std::map<std::string, PropertyDataType>& propertyDataTypeMap = customRanker->getPropertyDataTypeMap();
    std::map<std::string, PropertyDataType>::iterator iter;
    for (iter = propertyDataTypeMap.begin(); iter != propertyDataTypeMap.end(); iter++)
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
        distSearchInfo.sortPropertyList_.push_back(std::make_pair(SortPropertyName, pSortProperty->isReverse()));

        if (SortPropertyName == "CUSTOM_RANK" || SortPropertyName == "RANK")
            continue;

        boost::shared_ptr<PropertyData> propData = getPropertyData_(SortPropertyName);
        if (! propData)
            continue;

        void* data = propData->data_;
        switch (propData->type_)
        {
        case INT_PROPERTY_TYPE:
        {
            std::vector<int64_t> dataList(docNum);
            for (size_t i = 0; i < docNum; i++)
            {
                dataList[i] = ((int64_t*)data)[docIdList[i]];
            }
            distSearchInfo.sortPropertyIntDataList_.push_back(std::make_pair(SortPropertyName, dataList));
        }
        break;
        case UNSIGNED_INT_PROPERTY_TYPE:
        {
            std::vector<uint64_t> dataList(docNum);
            for (size_t i = 0; i < docNum; i++)
            {
                dataList[i] = ((uint64_t*)data)[docIdList[i]];
            }
            distSearchInfo.sortPropertyUIntDataList_.push_back(std::make_pair(SortPropertyName, dataList));
        }
        break;
        case FLOAT_PROPERTY_TYPE:
        {
            std::vector<float> dataList(docNum);
            for (size_t i = 0; i < docNum; i++)
            {
                dataList[i] = ((float*)data)[docIdList[i]];
            }
            distSearchInfo.sortPropertyFloatDataList_.push_back(std::make_pair(SortPropertyName, dataList));
        }
        break;
        case DOUBLE_PROPERTY_TYPE:
        {
            std::vector<float> dataList(docNum);
            for (size_t i = 0; i < docNum; i++)
            {
                dataList[i] = (float)((double*)data)[docIdList[i]];
            }
            distSearchInfo.sortPropertyFloatDataList_.push_back(std::make_pair(SortPropertyName, dataList));
        }
        break;
        default:
            break;
        }
    }

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
    cout << "\n[Index terms info for Query]" << endl;
    DocumentFrequencyInProperties::iterator dfiter;
    for (dfiter = dfmap.begin(); dfiter != dfmap.end(); dfiter++)
    {
        cout << "property: " << dfiter->first << endl;
        ID_FREQ_UNORDERED_MAP_T::iterator iter_;
        for (iter_ = dfiter->second.begin(); iter_ != dfiter->second.end(); iter_++)
        {
            cout << "termid: " << iter_->first << " DF: " << iter_->second << endl;
        }
    }
    cout << "-----------------------" << endl;
    CollectionTermFrequencyInProperties::iterator ctfiter;
    for (ctfiter = ctfmap.begin(); ctfiter != ctfmap.end(); ctfiter++)
    {
        cout << "property: " << ctfiter->first << endl;
        ID_FREQ_UNORDERED_MAP_T::iterator iter_;
        for (iter_ = ctfiter->second.begin(); iter_ != ctfiter->second.end(); iter_++)
        {
            cout << "termid: " << iter_->first << " CTF: " << iter_->second << endl;
        }
    }
    cout << "-----------------------" << endl;
}

void SearchManager::setGroupFilterBuilder(
    faceted::GroupFilterBuilder* builder)
{
    groupFilterBuilder_.reset(builder);
}

} // namespace sf1r
