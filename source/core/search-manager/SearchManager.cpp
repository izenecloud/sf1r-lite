#include <bundles/index/IndexBundleConfiguration.h>
#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>
#include <query-manager/ActionItem.h>
#include <ranking-manager/RankingManager.h>
#include <mining-manager/MiningManager.h>
#include <mining-manager/faceted-submanager/ctr_manager.h>
#include <mining-manager/group-manager/GroupFilterBuilder.h>
#include <mining-manager/group-manager/GroupFilter.h>
#include <mining-manager/group-manager/PropSharedLockSet.h>
#include <mining-manager/faceted-submanager/ontology_rep.h>
#include <mining-manager/custom-rank-manager/CustomRankManager.h>
#include <mining-manager/product-scorer/RelevanceScorer.h>
#include <mining-manager/product-scorer/ProductScorerFactory.h>
#include <mining-manager/product-ranker/ProductRankerFactory.h>
#include <mining-manager/product-ranker/ProductRanker.h>
#include <mining-manager/product-ranker/ProductRankParam.h>
#include <common/SFLogger.h>

#include "SearchManager.h"
#include "CustomRanker.h"
#include "QueryBuilder.h"
#include "Sorter.h"
#include "HitQueue.h"
#include "FilterDocumentIterator.h"
#include "AllDocumentIterator.h"
#include "CombinedDocumentIterator.h"
#include "CustomRankDocumentIterator.h"
#include "SearchManagerPreProcessor.h"
#include "ScoreDocEvaluator.h"

#include <util/swap.h>
#include <util/get.h>
#include <util/ClockTimer.h>

#include <boost/algorithm/string.hpp>

#include <fstream>
#include <algorithm>
#include <memory> // auto_ptr
#include <omp.h>
#include <util/cpu_topology.h>

#define PARALLEL_THRESHOLD 80000

namespace sf1r
{

static izenelib::util::CpuTopologyT  s_cpu_topology_info;
static int  s_round = 0;
static int  s_cpunum = 0;

SearchManager::SearchManager(
    const IndexBundleSchema& indexSchema,
    const boost::shared_ptr<IDManager>& idManager,
    const boost::shared_ptr<DocumentManager>& documentManager,
    const boost::shared_ptr<IndexManager>& indexManager,
    const boost::shared_ptr<RankingManager>& rankingManager,
    IndexBundleConfiguration* config
)
    : config_(config)
    , indexManagerPtr_(indexManager)
    , documentManagerPtr_(documentManager)
    , rankingManagerPtr_(rankingManager)
    , queryBuilder_()
    , filter_hook_(0)
    , customRankManager_(NULL)
    , threadpool_(0)
    , preprocessor_(new SearchManagerPreProcessor())
    , productRankerFactory_(NULL)
{
    collectionName_ = config->collectionName_;
    for (IndexBundleSchema::const_iterator iter = indexSchema.begin();
            iter != indexSchema.end(); ++iter)
    {
        preprocessor_->schemaMap_[iter->getName()] = *iter;
    }

    pSorterCache_ = new SortPropertyCache(documentManagerPtr_.get(), indexManagerPtr_.get(), config);
    queryBuilder_.reset(new QueryBuilder(
                            indexManager,
                            documentManager,
                            idManager,
                            rankingManager,
                            preprocessor_->schemaMap_,
                            config->filterCacheNum_));
    rankingManagerPtr_->getPropertyWeightMap(propertyWeightMap_);
    izenelib::util::CpuInfo::InitCpuTopologyInfo(s_cpu_topology_info);
    if(s_cpunum == 0)
    {
        s_cpunum = omp_get_num_procs();
        if(s_cpunum <= 0)
        {
            s_cpunum = 1;
        }
    }
}

SearchManager::~SearchManager()
{
    if (pSorterCache_)
        delete pSorterCache_;
    delete preprocessor_;
}

void SearchManager::setCustomRankManager(CustomRankManager* customRankManager)
{
    customRankManager_ = customRankManager;
}

void SearchManager::setProductScorerFactory(ProductScorerFactory* productScorerFactory)
{
    preprocessor_->productScorerFactory_ = productScorerFactory;
}

void SearchManager::setProductRankerFactory(ProductRankerFactory* productRankerFactory)
{
    productRankerFactory_ = productRankerFactory;
}

bool SearchManager::rerank(
    const KeywordSearchActionItem& actionItem,
    KeywordSearchResult& resultItem)
{
    if (productRankerFactory_ &&
            resultItem.topKCustomRankScoreList_.empty() &&
            preprocessor_->isNeedRerank(actionItem))
    {
        izenelib::util::ClockTimer timer;

        ProductRankParam rankParam(resultItem.topKDocs_,
                                   resultItem.topKRankScoreList_,
                                   actionItem.isRandomRank_);

        boost::scoped_ptr<ProductRanker> productRanker(
            productRankerFactory_->createProductRanker(rankParam));

        productRanker->rank();

        LOG(INFO) << "topK doc num: " << resultItem.topKDocs_.size()
                  << ", product rerank costs: " << timer.elapsed()
                  << " seconds";

        return true;
    }

    return false;
}

void SearchManager::reset_all_property_cache()
{
    pSorterCache_->setDirty(true);
}

void SearchManager::reset_filter_cache()
{
    queryBuilder_->reset_cache();
}

void SearchManager::setGroupFilterBuilder(faceted::GroupFilterBuilder* builder)
{
    groupFilterBuilder_.reset(builder);
}

void SearchManager::setMiningManager(
    boost::shared_ptr<MiningManager> miningManagerPtr)
{
    miningManagerPtr_ = miningManagerPtr;
}

boost::shared_ptr<NumericPropertyTableBase>& SearchManager::createPropertyTable(const std::string& propertyName)
{
    return preprocessor_->createPropertyTable(propertyName, pSorterCache_);
}

struct SearchThreadParam
{
    SearchKeywordOperation* actionOperation;
    std::size_t totalCount_thread;
    sf1r::PropertyRange propertyRange;
    uint32_t start;
    boost::shared_ptr<Sorter> pSorter;
    CustomRankerPtr customRanker;
    faceted::GroupRep groupRep_thread;
    faceted::OntologyRep attrRep_thread;
    boost::shared_ptr<HitQueue>* scoreItemQueue;
    DistKeywordSearchInfo* distSearchInfo;
    std::map<std::string, unsigned int> counterResults_thread;
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
    std::map<std::string, unsigned int>& counterResults,
    uint32_t topK,
    uint32_t knnTopK,
    uint32_t knnDist,
    uint32_t start,
    bool enable_parallel_searching)
{
    if ( actionOperation.noError() == false )
        return false;

    docid_t maxDocId = documentManagerPtr_->getMaxDocId();
    size_t thread_num = s_cpunum;

    int running_node = 0;
    if(s_cpu_topology_info.cpu_topology_supported)
    {
        running_node = ++s_round%s_cpu_topology_info.cpu_topology_array.size();
        thread_num = s_cpu_topology_info.cpu_topology_array[running_node].size();
    }

    if(enable_parallel_searching && s_cpu_topology_info.cpu_topology_supported)
    {
        threadpool_.size_controller().resize(s_cpunum*4);
    }

    if(!enable_parallel_searching || (maxDocId < PARALLEL_THRESHOLD) )
        thread_num = 1;

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

    //struct timeval tv_start;
    //struct timeval tv_end;
    //gettimeofday(&tv_start, NULL);
    // begin thread code
    //
    std::vector<SearchThreadParam> searchparams;
    if(thread_num > 1)
    {
        searchparams.resize(thread_num);
        // make each thread process 2 separated data will get the most efficient.
        std::size_t docid_num_byeachthread = maxDocId/thread_num;
        for (size_t i = 0; i < thread_num; ++i)
        {
            searchparams[i].actionOperation               =    &actionOperation;
            searchparams[i].totalCount_thread             =    0;
            searchparams[i].start                         =    start;
            searchparams[i].propertyRange                 =    propertyRange;
            //searchparams[i].pSorter                       =    NULL;
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
            boost::detail::atomic_count finishedJobs(0);
            #pragma omp parallel for schedule(dynamic, 2)
            for(size_t i = 0; i < thread_num; ++i)
            {
                doSearchInThreadOneParam(&searchparams[i], &finishedJobs);
            }
        }
        else
        {
            boost::detail::atomic_count finishedJobs(0);
            for(size_t i = 0; i < thread_num; ++i)
            {
                threadpool_.schedule(boost::bind(&SearchManager::doSearchInThreadOneParam,
                                                 this, &searchparams[i], &finishedJobs));

            }
            threadpool_.wait(finishedJobs, thread_num);
        }
    }
    else
    {
        assert(thread_num == 1);
        try
        {
            if( !doSearchInThread(
                        actionOperation,
                        totalCount,
                        propertyRange,
                        start,
                        pSorter,
                        customRanker,
                        groupRep,
                        attrRep,
                        scoreItemQueue[0],
                        distSearchInfo,
                        counterResults,
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
        counterResults = searchparams[0].counterResults_thread;
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

            std::map<std::string,unsigned>::iterator cit = searchparams[i].counterResults_thread.begin();
            for(; cit != searchparams[i].counterResults_thread.end(); ++cit)
            {
                counterResults[cit->first] += cit->second;
            }
        }
    }
    size_t count = start > 0 ? (finalResultQueue->size() - start) : finalResultQueue->size();
    docIdList.resize(count);
    rankScoreList.resize(count);
    if (customRanker)
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
        if (customRanker)
        {
            // should not be normalized
            customRankScoreList[count - i - 1] = pScoreItem.custom_score;
        }
    }

    try
    {
        if(thread_num > 1)
        {
            std::list<faceted::OntologyRep*> all_attrReps;
            for(size_t i = 0; i < thread_num; ++i)
            {
                groupRep.merge(searchparams[i].groupRep_thread);
                all_attrReps.push_back(&searchparams[i].attrRep_thread);
            }
            attrRep.merge(actionOperation.actionItem_.groupParam_.attrGroupNum_,
                          all_attrReps);
        }
        if (distSearchInfo.effective_ && pSorter)
        {
            // all sorters will be the same after searching, so we can just use any sorter.
            fillSearchInfoWithSortPropertyData_(pSorter.get(), docIdList, distSearchInfo);
        }
    }
    catch(std::exception& e)
    {
        return false;
    }

    //gettimeofday(&tv_end, NULL);
    //double timespend = (double) tv_end.tv_sec - (double) tv_start.tv_sec
    //                   + ((double) tv_end.tv_usec - (double) tv_start.tv_usec) / 1000000;
    //std::cout << "In thread: " << (long)pthread_self() << "searching by threading all cost " << timespend << " seconds." << std::endl;

    REPORT_PROFILE_TO_SCREEN();
    return true;
}

void SearchManager::doSearchInThreadOneParam(SearchThreadParam* pParam,
        boost::detail::atomic_count* finishedJobs
                                            )
{

    assert(pParam);
    if(s_cpu_topology_info.cpu_topology_supported)
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(s_cpu_topology_info.cpu_topology_array[pParam->running_node][pParam->docid_start/pParam->docid_num_byeachthread], &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
    }

    //struct timeval tv_start;
    //struct timeval tv_end;
    //gettimeofday(&tv_start, NULL);

    if(pParam)
    {
        pParam->ret = doSearchInThread(*(pParam->actionOperation),
                                       pParam->totalCount_thread, pParam->propertyRange, pParam->start,
                                       pParam->pSorter, pParam->customRanker, pParam->groupRep_thread, pParam->attrRep_thread,
                                       *(pParam->scoreItemQueue), *(pParam->distSearchInfo), pParam->counterResults_thread, pParam->heapSize,
                                       pParam->docid_start, pParam->docid_num_byeachthread, pParam->docid_nextstart_inc, true);
    }
    ++(*finishedJobs);
    //gettimeofday(&tv_end, NULL);
    //double timespend = (double) tv_end.tv_sec - (double) tv_start.tv_sec
    //                   + ((double) tv_end.tv_usec - (double) tv_start.tv_usec) / 1000000;
    //std::cout << "==== In worker thread: " << (long)pthread_self() << "searching cost " << timespend << " seconds." << std::endl;
    //if(pParam->ret)
    //{
    //    std::cout << "thread: " << (long)pthread_self() << " result: topK-" <<
    //        (*(pParam->scoreItemQueue))->size() << ", total-"<< pParam->totalCount_thread << endl;
    //}
}

bool SearchManager::doSearchInThread(const SearchKeywordOperation& actionOperation,
                                     std::size_t& totalCount_orig,
                                     sf1r::PropertyRange& propertyRange_orig,
                                     uint32_t start,
                                     boost::shared_ptr<Sorter>& pSorter_orig,
                                     CustomRankerPtr& customRanker_orig,
                                     faceted::GroupRep& groupRep,
                                     faceted::OntologyRep& attrRep,
                                     boost::shared_ptr<HitQueue>& scoreItemQueue_orig,
                                     DistKeywordSearchInfo& distSearchInfo,
                                     std::map<std::string, unsigned int>& counterResults,
                                     int heapSize,
                                     std::size_t docid_start,
                                     std::size_t docid_num_byeachthread,
                                     std::size_t docid_nextstart_inc,
                                     bool is_parallel
                                    )
{
    CREATE_PROFILER( preparedociter, "SearchManager", "doSearch_: SearchManager_search : build doc iterator");
    CREATE_PROFILER( preparerank, "SearchManager", "doSearch_: prepare ranker");

    unsigned int collectionId = 1;
    std::vector<std::string> indexPropertyList( actionOperation.actionItem_.searchPropertyList_ );
    START_PROFILER( preparedociter )
    unsigned indexPropertySize = indexPropertyList.size();
    std::vector<propertyid_t> indexPropertyIdList(indexPropertySize);

    preprocessor_->PreparePropertyList(indexPropertyList, indexPropertyIdList,
                                       boost::bind(&QueryBuilder::getPropertyIdByName, queryBuilder_.get(), _1));

    // build term index maps
    const property_term_info_map& propertyTermInfoMap = actionOperation.getPropertyTermInfoMap();
    std::vector<std::map<termid_t, unsigned> > termIndexMaps(indexPropertySize);
    preprocessor_->PreparePropertyTermIndex(propertyTermInfoMap,
                                            indexPropertyList, termIndexMaps);


    sf1r::TextRankingType& pTextRankingType = actionOperation.actionItem_.rankingType_;
    bool isWandStrategy = (actionOperation.actionItem_.searchingMode_.mode_ == SearchingMode::WAND);
    bool isTFIDFModel = (pTextRankingType == RankingType::BM25
                         ||config_->rankingManagerConfig_.rankingConfigUnit_.textRankingModel_ == RankingType::BM25);
    bool isWandSearch = (isWandStrategy && isTFIDFModel && (!actionOperation.isPhraseOrWildcardQuery_));
    bool useOriginalQuery = actionOperation.actionItem_.searchingMode_.useOriginalQuery_;

    std::vector<boost::shared_ptr<PropertyRanker> > propertyRankers;
    rankingManagerPtr_->createPropertyRankers(pTextRankingType, indexPropertySize, propertyRankers);
    bool readTermPosition = propertyRankers[0]->requireTermPosition();

    std::auto_ptr<DocumentIterator> scoreDocIterPtr;
    MultiPropertyScorer* pMultiPropertyIterator = NULL;
    WANDDocumentIterator* pWandDocIterator = NULL;

    std::vector<QueryFiltering::FilteringType>& filtingList =
        actionOperation.actionItem_.filteringList_;
    boost::shared_ptr<EWAHBoolArray<uint32_t> > pFilterIdSet;

    // when query is "*"
    const bool isFilterQuery =
        actionOperation.rawQueryTree_->type_ == QueryTree::FILTER_QUERY;

    try
    {
        if (filter_hook_)
            filter_hook_(filtingList);

        if (!filtingList.empty())
            queryBuilder_->prepare_filter(filtingList, pFilterIdSet);

        if (isFilterQuery == false)
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
                scoreDocIterPtr.reset(pWandDocIterator);
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
                scoreDocIterPtr.reset(pMultiPropertyIterator);
            }
        }
    }
    catch (std::exception& e)
    {
        return false;
    }

    boost::shared_ptr<CombinedDocumentIterator> pDocIterator(
        new CombinedDocumentIterator());
    if (pFilterIdSet)
    {
        ///1. Search Filter
        ///2. Select * WHERE    (FilterQuery)
        BitMapIterator* pBitmapIter = new BitMapIterator(pFilterIdSet);
        FilterDocumentIterator* pFilterIterator = new FilterDocumentIterator(pBitmapIter);
        pDocIterator->add(pFilterIterator);
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

    DocumentIterator* pScoreDocIterator = scoreDocIterPtr.get();
    if (isFilterQuery == false)
    {
        pScoreDocIterator = combineCustomDocIterator_(
            actionOperation.actionItem_, scoreDocIterPtr.release());

        if (pScoreDocIterator == NULL)
        {
            const std::string& query =
                actionOperation.actionItem_.env_.queryString_;

            LOG(INFO) << "empty search result for query [" << query << "]";
            return false;
        }

        pDocIterator->add(pScoreDocIterator);
    }

    if (isFilterQuery && !pFilterIdSet)
    {
        //SELECT * , and filter is null
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
            distSearchInfo.dfmap_.swap(dfmap);
            distSearchInfo.ctfmap_.swap(ctfmap);
            distSearchInfo.maxtfmap_.swap(maxtfmap);
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
    if (pWandDocIterator)
    {
        for (size_t i = 0; i < indexPropertyList.size(); ++i )
        {
            const std::string& currentProperty = indexPropertyList[i];
            ID_FREQ_MAP_T& ub = ubmap[currentProperty];
            propertyRankers[i]->calculateTermUBs(rankQueryProperties[i], ub);
        }
        pWandDocIterator->set_ub( useOriginalQuery, ubmap );
        pWandDocIterator->init_threshold(actionOperation.actionItem_.searchingMode_.threshold_);
    }

    STOP_PROFILER( preparerank )

    faceted::PropSharedLockSet propSharedLockSet;
    boost::scoped_ptr<faceted::GroupFilter> groupFilter;
    faceted::GroupParam gp = actionOperation.actionItem_.groupParam_;
    if (groupFilterBuilder_)
    {
        if(is_parallel)
        {
            // Because the top info can not be decided until
            // all threads' result merged, we need to get all the
            // attribute info in each thread.
            gp.attrGroupNum_ = 0;
        }
        groupFilter.reset(
            groupFilterBuilder_->createFilter(gp, propSharedLockSet));
    }
    std::size_t totalCount;
    sf1r::PropertyRange propertyRange = propertyRange_orig;

    ProductScorer* relevanceScorer = NULL;
    if (pScoreDocIterator)
    {
        relevanceScorer = new RelevanceScorer(*pScoreDocIterator,
                                              rankQueryProperties,
                                              propertyRankers);
    }

    ProductScorer* productScorer = preprocessor_->createProductScorer(
        actionOperation.actionItem_, propSharedLockSet, relevanceScorer);

    ScoreDocEvaluator scoreDocEvaluator(productScorer, customRanker);

    try
    {
        bool ret = doSearch_(
                       actionOperation,
                       totalCount,
                       propertyRange,
                       pDocIterator.get(),
                       groupFilter.get(),
                       scoreDocEvaluator,
                       scoreItemQueue.get(),
                       counterResults,
                       docid_start,
                       docid_num_byeachthread,
                       docid_nextstart_inc);
        // for CPU cache missing optimization, parallel searching need
        // to separate all the result containers to get the best performance.
        // That is minimize the access to the memory which is not allocated in
        // the self thread.
        scoreItemQueue_orig = scoreItemQueue;
        totalCount_orig = totalCount;
        propertyRange_orig = propertyRange;
        pSorter_orig = pSorter;
        customRanker_orig = customRanker;
        if(groupFilter)
        {
            faceted::GroupRep groupRep_thread;
            faceted::OntologyRep attrRep_thread;
            groupFilter->getGroupRep(groupRep_thread, attrRep_thread);
            groupRep.swap(groupRep_thread);
            attrRep.swap(attrRep_thread);
        }
        return ret;
    }
    catch (std::exception& e)
    {
        return false;
    }
    return false;
}

bool SearchManager::doSearch_(
    const SearchKeywordOperation& actionOperation,
    std::size_t& totalCount,
    sf1r::PropertyRange& propertyRange,
    CombinedDocumentIterator* pDocIterator,
    faceted::GroupFilter* groupFilter,
    ScoreDocEvaluator& scoreDocEvaluator,
    HitQueue* scoreItemQueue,
    std::map<std::string, unsigned>& counterResults,
    std::size_t docid_start,
    std::size_t docid_num_byeachthread,
    std::size_t docid_nextstart_inc)
{
    CREATE_PROFILER( computerankscore, "SearchManager", "doSearch_: overall time for scoring a doc");
    CREATE_PROFILER( inserttoqueue, "SearchManager", "doSearch_: overall time for inserting to result queue");
    typedef boost::shared_ptr<NumericPropertyTableBase> NumericPropertyTablePtr;
    totalCount = 0;
    const std::string& rangePropertyName = actionOperation.actionItem_.rangePropertyName_;
    NumericPropertyTablePtr rangePropertyTable;
    float lowValue = (std::numeric_limits<float>::max) ();
    float highValue = - lowValue;

    if (!rangePropertyName.empty())
    {
        rangePropertyTable = documentManagerPtr_->getNumericPropertyTable(rangePropertyName);
    }

    std::vector<NumericPropertyTablePtr> counterTables;
    std::vector<uint32_t> counterValues;
    unsigned counterSize = actionOperation.actionItem_.counterList_.size();
    unsigned ii = 0;
    if(counterSize)
    {
        counterTables.resize(counterSize);
        counterValues.resize(counterSize,0);
        for(; ii < counterSize; ++ii)
        {
            counterTables[ii] = documentManagerPtr_->getNumericPropertyTable(actionOperation.actionItem_.counterList_[ii]);
        }
    }

    if(docid_start > 0)
        pDocIterator->skipTo(docid_start);
    std::size_t docid_end = docid_start + docid_num_byeachthread;

    while (pDocIterator->next())
    {
        docid_t curDocId = pDocIterator->doc();

        if (groupFilter && !groupFilter->test(curDocId))
            continue;

        if (rangePropertyTable)
        {
            float docPropertyValue = 0;
            if (rangePropertyTable->getFloatValue(curDocId, docPropertyValue))
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

        /// COUNT(PropertyName)  semantics
        if (counterSize)
        {
            for(ii = 0; ii < counterSize; ++ii)
            {
                if(counterTables[ii]->isValid(curDocId))
                    ++counterValues[ii];
            }
        }

        ScoreDoc scoreItem(curDocId);

        START_PROFILER( computerankscore )

        ++totalCount;
        scoreDocEvaluator.evaluate(scoreItem);

        STOP_PROFILER( computerankscore )

        START_PROFILER( inserttoqueue )
        scoreItemQueue->insert(scoreItem);
        STOP_PROFILER( inserttoqueue )

        if(curDocId >= docid_end)
        {
            docid_start = docid_end + docid_nextstart_inc;
            if(docid_start > curDocId)
                pDocIterator->skipTo(docid_start);
            docid_end = docid_start + docid_num_byeachthread;
        }
    }

    if (rangePropertyTable && lowValue <= highValue)
    {
        propertyRange.highValue_ = highValue;
        propertyRange.lowValue_ = lowValue;
    }
    if (counterSize)
    {
        for(ii = 0; ii < counterSize; ++ii)
        {
            counterResults[actionOperation.actionItem_.counterList_[ii]] = counterValues[ii];
        }
    }
    return true;
}

void SearchManager::prepare_sorter_customranker_(
    const SearchKeywordOperation& actionOperation,
    CustomRankerPtr& customRanker,
    boost::shared_ptr<Sorter> &pSorter)
{
    preprocessor_->prepare_sorter_customranker_(actionOperation, customRanker, pSorter,
            pSorterCache_, miningManagerPtr_);
}

void SearchManager::fillSearchInfoWithSortPropertyData_(
    Sorter* pSorter,
    std::vector<unsigned int>& docIdList,
    DistKeywordSearchInfo& distSearchInfo)
{
    preprocessor_->fillSearchInfoWithSortPropertyData_(pSorter, docIdList, distSearchInfo, pSorterCache_);
}

DocumentIterator* SearchManager::combineCustomDocIterator_(
    const KeywordSearchActionItem& actionItem,
    DocumentIterator* originDocIterator)
{
    if (customRankManager_ &&
        preprocessor_->isNeedCustomDocIterator(actionItem))
    {
        const std::string& query = actionItem.env_.queryString_;
        CustomRankDocId customDocId;

        if (!customRankManager_->getCustomValue(query, customDocId) ||
                customDocId.empty())
        {
            return originDocIterator;
        }

        DocumentIterator* pCustomDocIter =
            new CustomRankDocumentIterator(customDocId);

        if (originDocIterator)
        {
            pCustomDocIter->add(originDocIterator);
        }

        return pCustomDocIter;
    }

    return originDocIterator;
}

void SearchManager::rankDocIdListForFuzzySearch(const SearchKeywordOperation& actionOperation,
        uint32_t start, std::vector<uint32_t>& docid_list, std::vector<float>& result_score_list,
        std::vector<float>& custom_score_list)
{
    if(docid_list.empty())
        return;
    CustomRankerPtr customRanker;
    boost::shared_ptr<Sorter> pSorter;
    try
    {
        prepare_sorter_customranker_(actionOperation, customRanker, pSorter);
    }
    catch (std::exception& e)
    {
        return;
    }

    faceted::PropSharedLockSet propSharedLockSet;
    boost::scoped_ptr<faceted::GroupFilter> groupFilter;
    faceted::GroupParam gp = actionOperation.actionItem_.groupParam_;
    if (groupFilterBuilder_)
    {
        groupFilter.reset(
            groupFilterBuilder_->createFilter(gp, propSharedLockSet));
    }

    ProductScorer* productScorer = preprocessor_->createProductScorer(
        actionOperation.actionItem_, propSharedLockSet, NULL);

    if (productScorer == NULL && !customRanker &&
       preprocessor_->isSortByRankProp(actionOperation.actionItem_.sortPriorityList_))
    {
        LOG(INFO) << "no need to resort, sorting by original fuzzy match order.";
        return;
    }

    ScoreDocEvaluator scoreDocEvaluator(productScorer, customRanker);
    const score_t fuzzyScoreWeight = getFuzzyScoreWeight_();

    const size_t count = docid_list.size();
    result_score_list.resize(count);
    custom_score_list.resize(count);

    boost::scoped_ptr<HitQueue> scoreItemQueue;
    ///sortby
    if (pSorter)
        scoreItemQueue.reset(new PropertySortedHitQueue(pSorter, count));
    else
        scoreItemQueue.reset(new ScoreSortedHitQueue(count));

    ScoreDoc tmpdoc;
    for(size_t i = 0; i < count; ++i)
    {
        tmpdoc.docId = docid_list[i];
        scoreDocEvaluator.evaluate(tmpdoc);

        float fuzzyScore = result_score_list[i];
        if(productScorer == NULL)
        {
            tmpdoc.score = fuzzyScore;
        }
        else
        {
            tmpdoc.score += fuzzyScore * fuzzyScoreWeight;
        }

        scoreItemQueue->insert(tmpdoc);
        //cout << "doc : " << tmpdoc.docId << ", score is:" << tmpdoc.score << "," << tmpdoc.custom_score << endl;
    }
    const size_t need_count = start > 0 ? (scoreItemQueue->size() - start) : scoreItemQueue->size();
    docid_list.resize(need_count);
    result_score_list.resize(need_count);
    custom_score_list.resize(need_count);
    for (size_t i = 0; i < need_count; ++i)
    {
        const ScoreDoc& pScoreItem = scoreItemQueue->pop();
        docid_list[need_count - i - 1] = pScoreItem.docId;
        result_score_list[need_count - i - 1] = pScoreItem.score;
        if (customRanker)
        {
            custom_score_list[need_count - i - 1] = pScoreItem.custom_score;
        }
    }
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

score_t SearchManager::getFuzzyScoreWeight_() const
{
    boost::shared_ptr<MiningManager> miningManager = miningManagerPtr_.lock();

    if (!miningManager)
    {
        LOG(WARNING) << "as SearchManager::miningManagerPtr_ is uninitialized, "
                     << "the fuzzy score weight would be zero";
        return 0;
    }

    const MiningSchema& miningSchema = miningManager->getMiningSchema();
    const ProductScoreConfig& fuzzyConfig =
        miningSchema.product_ranking_config.scores[FUZZY_SCORE];

    return fuzzyConfig.weight;
}

} // namespace sf1r
