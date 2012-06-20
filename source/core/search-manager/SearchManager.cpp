#include <bundles/index/IndexBundleConfiguration.h>
#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>
#include <query-manager/ActionItem.h>
#include <ranking-manager/RankingManager.h>
#include <mining-manager/MiningManager.h>
#include <mining-manager/faceted-submanager/ctr_manager.h>
#include <mining-manager/group-manager/GroupFilterBuilder.h>
#include <mining-manager/group-manager/GroupFilter.h>
#include <mining-manager/faceted-submanager/ontology_rep.h>
#include <mining-manager/product-ranker/ProductRankerFactory.h>
#include <mining-manager/product-ranker/ProductRanker.h>
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
#include <util/ClockTimer.h>

#include <boost/algorithm/string.hpp>

#include <fstream>
#include <algorithm>
#include <omp.h>
#include <util/cpu_topology.h>

#define PARALLEL_THRESHOLD 8000000

namespace sf1r
{

const std::string RANK_PROPERTY("_rank");
const std::string DATE_PROPERTY("date");
const std::string CUSTOM_RANK_PROPERTY("custom_rank");
const std::string CTR_PROPERTY("_ctr");

static izenelib::util::CpuTopologyT  s_cpu_topology_info;
static int  s_round = 0;

bool isProductRanking(const KeywordSearchActionItem& actionItem)
{
    if (actionItem.searchingMode_.mode_ == SearchingMode::KNN)
        return false;

    const KeywordSearchActionItem::SortPriorityList& sortPropertyList = actionItem.sortPriorityList_;
    if (sortPropertyList.empty())
        return true;

    for (KeywordSearchActionItem::SortPriorityList::const_iterator it = sortPropertyList.begin();
        it != sortPropertyList.end(); ++it)
    {
        std::string fieldNameL = it->first;
        boost::to_lower(fieldNameL);
        if (fieldNameL == RANK_PROPERTY)
            return true;
    }

    return false;
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
    , productRankerFactory_(NULL)
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
                            rankingManager,
                            schemaMap_,
                            config->filterCacheNum_));
    rankingManagerPtr_->getPropertyWeightMap(propertyWeightMap_);
    izenelib::util::CpuInfo::InitCpuTopologyInfo(s_cpu_topology_info);
}

SearchManager::~SearchManager()
{
    if (pSorterCache_)
        delete pSorterCache_;
}

void SearchManager::updateSortCache(
        docid_t id,
        const std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue)
{
    pSorterCache_->updateSortData(id, rTypeFieldValue);
}

void SearchManager::reset_all_property_cache()
{
    pSorterCache_->setDirty(true);

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
    if (productRankerFactory_ &&
        resultItem.topKCustomRankScoreList_.empty() &&
        isProductRanking(actionItem))
    {
        izenelib::util::ClockTimer timer;

        ProductRankingParam rankingParam(actionItem.env_.queryString_,
            resultItem.topKDocs_, resultItem.topKRankScoreList_);

        boost::scoped_ptr<ProductRanker> productRanker(
            productRankerFactory_->createProductRanker(rankingParam));

        productRanker->rank();

        LOG(INFO) << "topK doc num: " << resultItem.topKDocs_.size()
                  << ", product ranking costs: " << timer.elapsed() << " seconds";

        return true;
    }
    return false;
}

struct SearchThreadParam
{
    bool isWandSearch;
    SearchKeywordOperation* actionOperation;
    std::size_t totalCount_thread;
    sf1r::PropertyRange propertyRange;
    uint32_t start;
    std::vector<RankQueryProperty>* rankQueryProperties;
    std::vector<boost::shared_ptr<PropertyRanker> >* propertyRankers;
    boost::shared_ptr<Sorter> pSorter;
    CustomRankerPtr customRanker;
    MultiPropertyScorer* pMultiPropertyIterator;
    WANDDocumentIterator* pWandDocIterator;
    CombinedDocumentIterator* pDocIterator;
    boost::shared_ptr<faceted::GroupFilter>* groupFilter;
    boost::shared_ptr<HitQueue>* scoreItemQueue;
    DistKeywordSearchInfo* distSearchInfo;
    int heapSize;
    std::size_t docid_start;
    std::size_t docid_num_byeachthread;
    std::size_t docid_nextstart_inc;
    bool  ret;
    int  running_node;
};


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

    if ( actionOperation.noError() == false )
        return false;

    docid_t maxDocId = documentManagerPtr_->getMaxDocId();
    size_t thread_num = omp_get_num_procs();

    int running_node = 0;
    if(s_cpu_topology_info.cpu_topology_supported)
    {
        running_node = ++s_round%s_cpu_topology_info.cpu_topology_array.size();
        thread_num = s_cpu_topology_info.cpu_topology_array[running_node].size();
    }
    //thread_num = 1;

    if (distSearchInfo.effective_ && 
        (distSearchInfo.option_ == DistKeywordSearchInfo::OPTION_GATHER_INFO))
    {
        // gather distribute info do not need multi-threads.
        thread_num = 1;
    }

    CustomRankerPtr customRanker;
    boost::shared_ptr<Sorter> pSorter;
    std::vector< boost::shared_ptr<HitQueue> > scoreItemQueue;
    scoreItemQueue.resize(thread_num);

    int heapSize = topK + start;

    std::vector< boost::shared_ptr<faceted::GroupFilter> > groupFilters;
    groupFilters.resize(thread_num);

    struct timeval tv_start;
    struct timeval tv_end;
    gettimeofday(&tv_start, NULL);
    // begin thread code
    //
    std::vector< boost::shared_ptr<boost::thread> > searching_threads;
    std::vector<SearchThreadParam> searchparams;
    if(thread_num > 1)
    {
        searchparams.resize(thread_num);
        // make each thread process 2 separated data will get the most efficient.
        std::size_t  docid_num_byeachthread = maxDocId/thread_num;
        for(size_t i = 0; i < thread_num; ++i)
        {
            searchparams[i].actionOperation               =    &actionOperation;          
            searchparams[i].totalCount_thread             =    0;                     
            searchparams[i].start                         =    start;                     
            searchparams[i].propertyRange                 =    propertyRange;                     
            //searchparams[i].pSorter                       =    NULL;                     
            searchparams[i].groupFilter                   =    &groupFilters[i];     
            searchparams[i].scoreItemQueue                =    &scoreItemQueue[i];
            searchparams[i].distSearchInfo                =    &distSearchInfo;
            searchparams[i].heapSize                      =    heapSize;
            searchparams[i].docid_start                   =    i*docid_num_byeachthread;
            searchparams[i].docid_num_byeachthread        =    docid_num_byeachthread;                       
            searchparams[i].docid_nextstart_inc           =    docid_num_byeachthread*(thread_num - 1);                       
            searchparams[i].running_node                  =    running_node;                       
        }

        if(!s_cpu_topology_info.cpu_topology_supported)
        {
#pragma omp parallel for schedule(dynamic, 2)
            for(size_t i = 0; i < thread_num; ++i)
            {
                doSearchInThreadOneParam(&searchparams[i]);
            }
        }
        else
        {
            for(size_t i = 0; i < thread_num; ++i)
            {
                searching_threads.push_back(boost::shared_ptr<boost::thread>(new boost::thread( boost::bind( &SearchManager::doSearchInThreadOneParam, 
                                this, &searchparams[i]) ) ) );
            }
            for(size_t i = 0; i < thread_num; ++i)
            {
                searching_threads[i]->join();
            }
        }
    }
    else
    {
        assert(thread_num == 1);
        try{
            if( !doSearchInThread(
            actionOperation,
            totalCount,
            propertyRange,
            start,
            pSorter,
            customRanker,
            groupFilters[0],
            scoreItemQueue[0],
            distSearchInfo,
            heapSize,
            0,
            (std::size_t)maxDocId,
            (std::size_t)maxDocId) )
            {
                return false;
            }

            if (distSearchInfo.effective_ && 
                (distSearchInfo.option_ == DistKeywordSearchInfo::OPTION_GATHER_INFO))
            {
                return true;
            }
        }
        catch(std::exception& e)
        {
            return false;
        }
    }
    boost::shared_ptr<HitQueue> finalResultQueue;
    finalResultQueue = scoreItemQueue[0];
    if(thread_num > 1)
    {
        if(!(searchparams[0].ret))
            return false;

        pSorter = searchparams[0].pSorter;
        customRanker = searchparams[0].customRanker;

        propertyRange = searchparams[0].propertyRange;
        totalCount = searchparams[0].totalCount_thread;
        // merging the result;
        float lowValue = (std::numeric_limits<float>::max) ();
        float highValue = - lowValue;
        for(size_t i = 1; i < thread_num; ++i)
        {
            if(!(searchparams[i].ret))
                return false;

            totalCount += searchparams[i].totalCount_thread;
            while(scoreItemQueue[i]->size() > 0)
            {
                finalResultQueue->insert(scoreItemQueue[i]->pop());
            }

            lowValue = searchparams[i].propertyRange.lowValue_;
            highValue = searchparams[i].propertyRange.highValue_;

            if (lowValue <= highValue)
            {
                propertyRange.highValue_ = max(highValue, propertyRange.highValue_);
                propertyRange.lowValue_ = min(lowValue, propertyRange.lowValue_);
            }
        }
    }
    size_t count = start > 0 ? (finalResultQueue->size() - start) : finalResultQueue->size();
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
    for (size_t i = 0; i < count; ++i)
    {
        const ScoreDoc& pScoreItem = finalResultQueue->pop();
        docIdList[count - i - 1] = pScoreItem.docId;
        rankScoreList[count - i - 1] = pScoreItem.score;
        if (customRanker.get())
        {
            // should not be normalized
            customRankScoreList[count - i - 1] = pScoreItem.custom_score;
        }
    }

    try
    {
        for(size_t i = 0; i < thread_num; ++i)
        {
            faceted::GroupRep groupRep_thread;
            faceted::OntologyRep attrRep_thread;

            if (groupFilters[i])
                groupFilters[i]->getGroupRep(groupRep_thread, attrRep_thread);

            groupRep.merge(groupRep_thread);
            // how to merge the OntologyRep, these should be reviewed.
            //
            if (attrRep_thread.item_list.empty())
                continue;

            if (attrRep.item_list.empty())
            {
                attrRep.swap(attrRep_thread);
                continue;
            }
            faceted::OntologyRep::item_iterator it = attrRep_thread.Begin();
            faceted::OntologyRep::item_iterator endit = attrRep_thread.End();
            while(it != endit)
            {
                attrRep.item_list.push_back(*it);
                ++it;
            }

        }
        if (distSearchInfo.effective_ && pSorter)
        {
            // all sorters will be the same after searching, so we can just use any sorter.
            getSortPropertyData_(pSorter.get(), docIdList, distSearchInfo);
        }
    }
    catch(std::exception& e)
    {
        return false;
    }

    gettimeofday(&tv_end, NULL);
    double timespend = (double) tv_end.tv_sec - (double) tv_start.tv_sec
                       + ((double) tv_end.tv_usec - (double) tv_start.tv_usec) / 1000000;
    std::cout << "In thread: " << (long)pthread_self() << "searching by threading all cost " << timespend << " seconds." << std::endl;

    REPORT_PROFILE_TO_SCREEN();
    return true;
}

void SearchManager::doSearchInThreadOneParam(SearchThreadParam* pParam)
{

    assert(pParam);
    if(s_cpu_topology_info.cpu_topology_supported)
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(s_cpu_topology_info.cpu_topology_array[pParam->running_node][pParam->docid_start/pParam->docid_num_byeachthread], &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
    }

    struct timeval tv_start;
    struct timeval tv_end;
    gettimeofday(&tv_start, NULL);

    if(pParam)
    {
        pParam->ret = doSearchInThread(*(pParam->actionOperation), 
            pParam->totalCount_thread, pParam->propertyRange, pParam->start,
            pParam->pSorter, pParam->customRanker, *(pParam->groupFilter), 
            *(pParam->scoreItemQueue), *(pParam->distSearchInfo), pParam->heapSize,
            pParam->docid_start, pParam->docid_num_byeachthread, pParam->docid_nextstart_inc);
    }
    gettimeofday(&tv_end, NULL);

    double timespend = (double) tv_end.tv_sec - (double) tv_start.tv_sec
                       + ((double) tv_end.tv_usec - (double) tv_start.tv_usec) / 1000000;
    std::cout << "==== In worker thread: " << (long)pthread_self() << "searching cost " << timespend << " seconds." << std::endl;
    if(pParam->ret)
    {
        std::cout << "thread: " << (long)pthread_self() << " result: topK-" << 
            (*(pParam->scoreItemQueue))->size() << ", total-"<< pParam->totalCount_thread << endl;
    }

}

bool SearchManager::doSearchInThread(const SearchKeywordOperation& actionOperation,
        std::size_t& totalCount_orig,
        sf1r::PropertyRange& propertyRange_orig,
        uint32_t start,
        boost::shared_ptr<Sorter>& pSorter_orig,
        CustomRankerPtr& customRanker_orig,
        boost::shared_ptr<faceted::GroupFilter>& groupFilter_orig,
        boost::shared_ptr<HitQueue>& scoreItemQueue_orig,
        DistKeywordSearchInfo& distSearchInfo,
        int heapSize,
        std::size_t docid_start,
        std::size_t docid_num_byeachthread,
        std::size_t docid_nextstart_inc
        )
{
    unsigned int collectionId = 1;
    std::vector<std::string> indexPropertyList( actionOperation.actionItem_.searchPropertyList_ );
#if PREFETCH_TERMID
    std::stable_sort (indexPropertyList.begin(), indexPropertyList.end());
#endif
    START_PROFILER( preparedociter )
    sf1r::TextRankingType& pTextRankingType = actionOperation.actionItem_.rankingType_;
    unsigned indexPropertySize = indexPropertyList.size();
    std::vector<propertyid_t> indexPropertyIdList(indexPropertySize);
    std::transform(
            indexPropertyList.begin(),
            indexPropertyList.end(),
            indexPropertyIdList.begin(),
            boost::bind(&QueryBuilder::getPropertyIdByName, queryBuilder_.get(), _1)
    );


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


    MultiPropertyScorer* pMultiPropertyIterator;
    pMultiPropertyIterator = NULL;
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
                                             termIndexMaps );
            }

            if ( !pWandDocIterator )
            {
                if(pMultiPropertyIterator == NULL)
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

    boost::shared_ptr<CombinedDocumentIterator> pDocIterator;
    pDocIterator.reset(new CombinedDocumentIterator());
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

    CustomRankerPtr customRanker;
    boost::shared_ptr<Sorter> pSorter;
    boost::shared_ptr<HitQueue> scoreItemQueue;

    try
    {
        prepare_sorter_customranker_(actionOperation, customRanker, pSorter);
    }
    catch (std::exception& e)
    {
        return false;
    }
    ///sortby
    if (pSorter)
        scoreItemQueue.reset(new PropertySortedHitQueue(pSorter, heapSize));
    else
        scoreItemQueue.reset(new ScoreSortedHitQueue(heapSize));

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
            pDocIterator->add(pFilterIterator);
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

    // until now all the pDocIterators is the same, so we can just compute the df, ctf and maxtf use
    // any one.
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

    std::vector<RankQueryProperty> rankQueryProperties(indexPropertyList.size());

    queryBuilder_->post_prepare_ranker_(
            "",
            indexPropertyList,
            indexPropertyList.size(),
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
        for (size_t i = 0; i < indexPropertyList.size(); ++i )
        {
            const std::string& currentProperty = indexPropertyList[i];
            ID_FREQ_MAP_T& ub = ubmap[currentProperty];
            propertyRankers[i]->calculateTermUBs(rankQueryProperties[i], ub);
        }
        pWandDocIterator->set_ub( ubmap );
        pWandDocIterator->init_threshold(actionOperation.actionItem_.searchingMode_.threshold_);
    }

    STOP_PROFILER( preparerank )

    boost::shared_ptr<faceted::GroupFilter> groupFilter;
    if (groupFilterBuilder_)
    {
        groupFilter.reset(
            groupFilterBuilder_->createFilter(actionOperation.actionItem_.groupParam_));
    }
    std::size_t totalCount;
    sf1r::PropertyRange propertyRange = propertyRange_orig;
    try
    {
        bool ret = doSearch_(
            isWandSearch,
            actionOperation,
            totalCount,
            propertyRange,
            start,
            rankQueryProperties,
            propertyRankers,
            pSorter.get(),
            customRanker,
            pMultiPropertyIterator,
            pWandDocIterator,
            pDocIterator.get(),
            groupFilter.get(),
            scoreItemQueue.get(),
            heapSize,
            docid_start,
            docid_num_byeachthread,
            docid_nextstart_inc);
        groupFilter_orig = groupFilter;
        scoreItemQueue_orig = scoreItemQueue;
        totalCount_orig = totalCount;
        propertyRange_orig = propertyRange;
        pSorter_orig = pSorter;
        customRanker_orig = customRanker;
        return ret;
    }
    catch (std::exception& e)
    {
        return false;
    }
    return false;
}

bool SearchManager::doSearch_(
        bool isWandSearch,
        const SearchKeywordOperation& actionOperation,
        std::size_t& totalCount,
        sf1r::PropertyRange& propertyRange,
        uint32_t start,
        const std::vector<RankQueryProperty>& rankQueryProperties,
        const std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers,
        Sorter* pSorter,
        CustomRankerPtr customRanker,
        MultiPropertyScorer* pMultiPropertyIterator,
        WANDDocumentIterator* pWandDocIterator,
        CombinedDocumentIterator* pDocIterator,
        faceted::GroupFilter* groupFilter,
        HitQueue* scoreItemQueue,
        int heapSize,
        std::size_t docid_start,
        std::size_t docid_num_byeachthread,
        std::size_t docid_nextstart_inc)
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

    if(docid_start > 0)
        pDocIterator->skipTo(docid_start);
    std::size_t docid_end = docid_start + docid_num_byeachthread;
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
        //if( isWandSearch && scoreItemQueue->size() == (unsigned)heapSize )
        //    pWandDocIterator->set_threshold((scoreItemQueue->top()).score);
        STOP_PROFILER( inserttoqueue )

        if(pDocIterator->doc() >= docid_end)
        {
            docid_start = docid_end + docid_nextstart_inc;
            if(docid_start > pDocIterator->doc())
                pDocIterator->skipTo(docid_start);
            docid_end = docid_start + docid_num_byeachthread;
        }
    }

    if (rangePropertyTable && lowValue <= highValue)
    {
        propertyRange.highValue_ = highValue;
        propertyRange.lowValue_ = lowValue;
    }

    //size_t count = start > 0 ? (scoreItemQueue->size() - start) : scoreItemQueue->size();

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
    return true;
}

void SearchManager::prepare_sorter_customranker_(
    const SearchKeywordOperation& actionOperation,
    CustomRankerPtr& customRanker,
    boost::shared_ptr<Sorter> &pSorter)
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

                if (!pSorter) pSorter.reset(new Sorter(pSorterCache_));
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
                if (!pSorter) pSorter.reset(new Sorter(pSorterCache_));
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
                if (!pSorter) pSorter.reset(new Sorter(pSorterCache_));
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
                if (!pSorter) pSorter.reset(new Sorter(pSorterCache_));

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
                if (!pSorter) pSorter.reset(new Sorter(pSorterCache_));
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
