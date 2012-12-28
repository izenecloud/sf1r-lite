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
#include <mining-manager/custom-rank-manager/CustomRankManager.h>
#include <mining-manager/product-scorer/RelevanceScorer.h>
#include <mining-manager/product-scorer/ProductScorerFactory.h>
#include <mining-manager/product-ranker/ProductRankerFactory.h>
#include <mining-manager/product-ranker/ProductRanker.h>
#include <mining-manager/product-ranker/ProductRankParam.h>
#include <common/PropSharedLockSet.h>
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
#include "SearchThreadParam.h"

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
    , isParallelEnabled_(config->enable_parallel_searching_)
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

bool SearchManager::search(
    const SearchKeywordOperation& actionOperation,
    KeywordSearchResult& searchResult,
    std::size_t limit,
    std::size_t offset)
{
    if (!actionOperation.noError())
        return false;

    const std::size_t heapSize = limit + offset;
    std::vector<SearchThreadParam> threadParams;
    DistKeywordSearchInfo& distSearchInfo = searchResult.distSearchInfo_;

    prepareThreadParams_(actionOperation,
                         distSearchInfo,
                         heapSize,
                         threadParams);

    if (!runThreadParams_(threadParams))
        return false;

    if (distSearchInfo.isOptionGatherInfo())
        return true;

    if (!mergeThreadParams_(threadParams))
        return false;

    if (!fetchSearchResult_(offset, threadParams.front(), searchResult))
        return false;

    REPORT_PROFILE_TO_SCREEN();
    return true;
}

void SearchManager::prepareThreadParams_(
    const SearchKeywordOperation& actionOperation,
    DistKeywordSearchInfo& distSearchInfo,
    std::size_t heapSize,
    std::vector<SearchThreadParam>& threadParams)
{
    std::size_t threadNum = 0;
    std::size_t runningNode = 0;
    getThreadInfo_(distSearchInfo, threadNum, runningNode);

    // in order to split the whole docid range [0, maxDocId+1) to N threads,
    // the average docs num for each thread is round_up((maxDocId+1)/N),
    // which equals to round_down(maxDocId/N) + 1,
    // then the docid range for each thread is [i*avg, (i+1)*avg).
    docid_t maxDocId = documentManagerPtr_->getMaxDocId();
    std::size_t averageDocNum = maxDocId/threadNum + 1;

    const SearchThreadParam initParam(&actionOperation,
                                      &distSearchInfo,
                                      heapSize,
                                      runningNode);
    threadParams.resize(threadNum, initParam);

    for (std::size_t i = 0; i < threadNum; ++i)
    {
        threadParams[i].threadId   = i;
        threadParams[i].docIdBegin = i * averageDocNum;
        threadParams[i].docIdEnd   = (i+1) * averageDocNum;
    }

    if (threadNum > 1)
    {
        // Because the top attribute info can not be decided until
        // all threads' result merged, we need to get all the
        // attribute info in each thread.
        std::swap(actionOperation.actionItem_.groupParam_.attrGroupNum_,
                  threadParams[0].originAttrGroupNum);
    }
}

void SearchManager::getThreadInfo_(
    const DistKeywordSearchInfo& distSearchInfo,
    std::size_t& threadNum,
    std::size_t& runningNode)
{
    docid_t maxDocId = documentManagerPtr_->getMaxDocId();
    threadNum = s_cpunum;
    runningNode = 0;

    if (s_cpu_topology_info.cpu_topology_supported)
    {
        runningNode = ++s_round % s_cpu_topology_info.cpu_topology_array.size();
        threadNum = s_cpu_topology_info.cpu_topology_array[runningNode].size();
    }

    if (isParallelEnabled_ && s_cpu_topology_info.cpu_topology_supported)
    {
        threadpool_.size_controller().resize(s_cpunum*4);
    }

    if (!isParallelEnabled_ ||
        maxDocId < PARALLEL_THRESHOLD ||
        distSearchInfo.isOptionGatherInfo())
    {
        threadNum = 1;
    }
}

bool SearchManager::runThreadParams_(
    std::vector<SearchThreadParam>& threadParams)
{
    if (threadParams.size() == 1)
        return runSingleThread_(threadParams.front());

    return runMultiThreads_(threadParams);
}

bool SearchManager::runSingleThread_(
    SearchThreadParam& threadParam)
{
    bool result = false;

    try
    {
        result = doSearchInThread(threadParam);
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << e.what();
    }

    return result;
}

bool SearchManager::runMultiThreads_(
    std::vector<SearchThreadParam>& threadParams)
{
    const std::size_t threadNum = threadParams.size();

    if (!s_cpu_topology_info.cpu_topology_supported)
    {
        boost::detail::atomic_count finishedJobs(0);
        #pragma omp parallel for schedule(dynamic, 2)
        for (std::size_t i = 0; i < threadNum; ++i)
        {
            doSearchInThreadOneParam(&threadParams[i], &finishedJobs);
        }
    }
    else
    {
        boost::detail::atomic_count finishedJobs(0);
        for (std::size_t i = 0; i < threadNum; ++i)
        {
            threadpool_.schedule(
                boost::bind(&SearchManager::doSearchInThreadOneParam,
                            this, &threadParams[i], &finishedJobs));

        }
        threadpool_.wait(finishedJobs, threadNum);
    }

    return true;
}

bool SearchManager::mergeThreadParams_(
    std::vector<SearchThreadParam>& threadParams) const
{
    const std::size_t threadNum = threadParams.size();
    if (threadNum == 1)
        return true;

    SearchThreadParam& masterParam = threadParams[0];
    if (!masterParam.isSuccess)
        return false;

    std::size_t& masterTotalCount = masterParam.totalCount;
    PropertyRange& masterPropertyRange = masterParam.propertyRange;
    std::map<std::string, unsigned int>& masterCounterResults = masterParam.counterResults;
    faceted::GroupRep& masterGroupRep = masterParam.groupRep;
    faceted::OntologyRep& masterAttrRep = masterParam.attrRep;
    std::list<faceted::OntologyRep*> otherAttrReps;
    boost::shared_ptr<HitQueue> masterQueue(masterParam.scoreItemQueue);

    for (std::size_t i = 1; i < threadNum; ++i)
    {
        SearchThreadParam& param = threadParams[i];
        if (!param.isSuccess)
            return false;

        masterTotalCount += param.totalCount;
        while (param.scoreItemQueue->size() > 0)
        {
            masterQueue->insert(param.scoreItemQueue->pop());
        }

        float lowValue = param.propertyRange.lowValue_;
        float highValue = param.propertyRange.highValue_;

        if (lowValue <= highValue)
        {
            masterPropertyRange.highValue_ =
                std::max(highValue, masterPropertyRange.highValue_);

            masterPropertyRange.lowValue_ =
                std::min(lowValue, masterPropertyRange.lowValue_);
        }

        for (std::map<std::string, unsigned>::iterator cit = param.counterResults.begin();
             cit != param.counterResults.end(); ++cit)
        {
            masterCounterResults[cit->first] += cit->second;
        }

        masterGroupRep.merge(param.groupRep);
        otherAttrReps.push_back(&param.attrRep);
    }

    int& attrGroupNum = masterParam.actionOperation->actionItem_.groupParam_.attrGroupNum_;
    std::swap(attrGroupNum, masterParam.originAttrGroupNum);
    masterAttrRep.merge(attrGroupNum, otherAttrReps);

    return true;
}

bool SearchManager::fetchSearchResult_(
    std::size_t offset,
    SearchThreadParam& threadParam,
    KeywordSearchResult& searchResult)
{
    std::swap(searchResult.totalCount_, threadParam.totalCount);
    searchResult.groupRep_.swap(threadParam.groupRep);
    searchResult.attrRep_.swap(threadParam.attrRep);
    searchResult.propertyRange_.swap(threadParam.propertyRange);
    searchResult.counterResults_.swap(threadParam.counterResults);

    std::vector<unsigned int>& docIdList = searchResult.topKDocs_;
    std::vector<float>& rankScoreList = searchResult.topKRankScoreList_;
    std::vector<float>& customRankScoreList = searchResult.topKCustomRankScoreList_;
    boost::shared_ptr<HitQueue> scoreItemQueue(threadParam.scoreItemQueue);
    CustomRankerPtr customRanker = threadParam.customRanker;

    const std::size_t count = scoreItemQueue->size() - offset;
    docIdList.resize(count);
    rankScoreList.resize(count);

    if (customRanker)
    {
        customRankScoreList.resize(count);
    }

    // here PriorityQueue is used to get topK elements, as PriorityQueue::pop()
    // will always get the element with current lowest score, we need to reverse
    // the output sequence.
    for (int i = count-1; i >= 0; --i)
    {
        const ScoreDoc& pScoreItem = scoreItemQueue->pop();
        docIdList[i] = pScoreItem.docId;
        rankScoreList[i] = pScoreItem.score;
        if (customRanker)
        {
            // should not be normalized
            customRankScoreList[i] = pScoreItem.custom_score;
        }
    }

    DistKeywordSearchInfo& distSearchInfo = searchResult.distSearchInfo_;
    boost::shared_ptr<Sorter> pSorter(threadParam.pSorter);
    if (distSearchInfo.effective_ && pSorter)
    {
        try
        {
            // all sorters will be the same after searching,
            // so we can just use any sorter.
            fillSearchInfoWithSortPropertyData_(pSorter.get(),
                                                docIdList,
                                                distSearchInfo);
        }
        catch (const std::exception& e)
        {
            LOG(ERROR) << e.what();
            return false;
        }
    }

    return true;
}

void SearchManager::doSearchInThreadOneParam(
    SearchThreadParam* pParam,
    boost::detail::atomic_count* finishedJobs)
{
    assert(pParam);
    if (s_cpu_topology_info.cpu_topology_supported)
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(s_cpu_topology_info.cpu_topology_array[pParam->runningNode][pParam->threadId], &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
    }

    if (pParam)
    {
        pParam->isSuccess = doSearchInThread(*pParam);
    }
    ++(*finishedJobs);
}

bool SearchManager::doSearchInThread(SearchThreadParam& pParam)
{
    CREATE_PROFILER( preparedociter, "SearchManager", "doSearch_: SearchManager_search : build doc iterator");
    CREATE_PROFILER( preparerank, "SearchManager", "doSearch_: prepare ranker");

    const SearchKeywordOperation& actionOperation = *pParam.actionOperation;

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

    try
    {
        prepare_sorter_customranker_(actionOperation, pParam.customRanker, pParam.pSorter);
    }
    catch (std::exception& e)
    {
        return false;
    }

    PropSharedLockSet propSharedLockSet;
    ///sortby
    if (pParam.pSorter)
    {
        pParam.scoreItemQueue.reset(new PropertySortedHitQueue(pParam.pSorter,
                                                               pParam.heapSize,
                                                               propSharedLockSet));
    }
    else
    {
        pParam.scoreItemQueue.reset(new ScoreSortedHitQueue(pParam.heapSize));
    }

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
        if (pParam.pSorter)
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
    DistKeywordSearchInfo& distSearchInfo = *pParam.distSearchInfo;
    if (distSearchInfo.effective_)
    {
        if (distSearchInfo.isOptionGatherInfo())
        {
            pDocIterator->df_cmtf(dfmap, ctfmap, maxtfmap);
            distSearchInfo.dfmap_.swap(dfmap);
            distSearchInfo.ctfmap_.swap(ctfmap);
            distSearchInfo.maxtfmap_.swap(maxtfmap);
            return true;
        }
        else if (distSearchInfo.isOptionCarriedInfo())
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

    boost::scoped_ptr<faceted::GroupFilter> groupFilter;
    if (groupFilterBuilder_)
    {
        groupFilter.reset(
            groupFilterBuilder_->createFilter(actionOperation.actionItem_.groupParam_,
                                              propSharedLockSet));
    }

    ProductScorer* relevanceScorer = NULL;
    if (pScoreDocIterator)
    {
        relevanceScorer = new RelevanceScorer(*pScoreDocIterator,
                                              rankQueryProperties,
                                              propertyRankers);
    }

    ProductScorer* productScorer = preprocessor_->createProductScorer(
        actionOperation.actionItem_, propSharedLockSet, relevanceScorer);

    ScoreDocEvaluator scoreDocEvaluator(productScorer, pParam.customRanker);

    try
    {
        bool ret = doSearch_(pParam,
                             pDocIterator.get(),
                             groupFilter.get(),
                             scoreDocEvaluator,
                             propSharedLockSet);

        if(groupFilter)
        {
            groupFilter->getGroupRep(pParam.groupRep, pParam.attrRep);
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
    SearchThreadParam& pParam,
    CombinedDocumentIterator* pDocIterator,
    faceted::GroupFilter* groupFilter,
    ScoreDocEvaluator& scoreDocEvaluator,
    PropSharedLockSet& propSharedLockSet)
{
    CREATE_PROFILER( computerankscore, "SearchManager", "doSearch_: overall time for scoring a doc");
    CREATE_PROFILER( inserttoqueue, "SearchManager", "doSearch_: overall time for inserting to result queue");

    const SearchKeywordOperation& actionOperation = *pParam.actionOperation;

    typedef boost::shared_ptr<NumericPropertyTableBase> NumericPropertyTablePtr;
    const std::string& rangePropertyName = actionOperation.actionItem_.rangePropertyName_;
    NumericPropertyTablePtr rangePropertyTable;
    float lowValue = (std::numeric_limits<float>::max) ();
    float highValue = - lowValue;

    if (!rangePropertyName.empty())
    {
        rangePropertyTable = documentManagerPtr_->getNumericPropertyTable(rangePropertyName);
        propSharedLockSet.insertSharedLock(rangePropertyTable.get());
    }

    std::vector<NumericPropertyTablePtr> counterTables;
    std::vector<uint32_t> counterValues;
    const std::vector<std::string>& counterList = actionOperation.actionItem_.counterList_;
    unsigned counterSize = counterList.size();
    unsigned ii = 0;
    if(counterSize)
    {
        counterTables.resize(counterSize);
        counterValues.resize(counterSize,0);
        for(; ii < counterSize; ++ii)
        {
            counterTables[ii] = documentManagerPtr_->getNumericPropertyTable(counterList[ii]);
            propSharedLockSet.insertSharedLock(counterTables[ii].get());
        }
    }

    pDocIterator->skipTo(pParam.docIdBegin);

    do
    {
        docid_t curDocId = pDocIterator->doc();

        if (curDocId >= pParam.docIdEnd)
            break;

        if (groupFilter && !groupFilter->test(curDocId))
            continue;

        if (rangePropertyTable)
        {
            float docPropertyValue = 0;
            if (rangePropertyTable->getFloatValue(curDocId, docPropertyValue, false))
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
                int32_t value;
                if(counterTables[ii]->getInt32Value(curDocId,value, false))
                {
                    counterValues[ii] += value;
                }
            }
        }

        ScoreDoc scoreItem(curDocId);

        START_PROFILER( computerankscore )

        ++pParam.totalCount;
        scoreDocEvaluator.evaluate(scoreItem);

        STOP_PROFILER( computerankscore )

        START_PROFILER( inserttoqueue )
        pParam.scoreItemQueue->insert(scoreItem);
        STOP_PROFILER( inserttoqueue )
    }
    while (pDocIterator->next());

    if (rangePropertyTable && lowValue <= highValue)
    {
        pParam.propertyRange.highValue_ = highValue;
        pParam.propertyRange.lowValue_ = lowValue;
    }
    if (counterSize)
    {
        for(ii = 0; ii < counterSize; ++ii)
        {
            const std::string& propName = counterList[ii];
            pParam.counterResults[propName] = counterValues[ii];
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

    PropSharedLockSet propSharedLockSet;
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

    const std::size_t count = docid_list.size();
    result_score_list.resize(count);
    custom_score_list.resize(count);

    boost::scoped_ptr<HitQueue> scoreItemQueue;
    ///sortby
    if (pSorter)
    {
        scoreItemQueue.reset(new PropertySortedHitQueue(pSorter,
                                                        count,
                                                        propSharedLockSet));
    }
    else
    {
        scoreItemQueue.reset(new ScoreSortedHitQueue(count));
    }

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
    const std::size_t need_count = start > 0 ? (scoreItemQueue->size() - start) : scoreItemQueue->size();
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
