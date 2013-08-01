#include "SearchThreadMaster.h"
#include "SearchThreadWorker.h"
#include "SearchManagerPreProcessor.h"
#include "SearchThreadParam.h"
#include "HitQueue.h"

#include <query-manager/SearchKeywordOperation.h>
#include <common/ResultType.h>
#include <document-manager/DocumentManager.h>
#include <bundles/index/IndexBundleConfiguration.h>

#include <omp.h>
#include <util/cpu_topology.h>

using namespace sf1r;

#define PARALLEL_THRESHOLD 80000

static izenelib::util::CpuTopologyT s_cpu_topology_info;
static int s_round = 0;
static int s_cpunum = 0;

SearchThreadMaster::SearchThreadMaster(
    const IndexBundleConfiguration& config,
    const boost::shared_ptr<DocumentManager>& documentManager,
    SearchManagerPreProcessor& preprocessor,
    SearchThreadWorker& searchThreadWorker)
    : isParallelEnabled_(config.enable_parallel_searching_)
    , documentManagerPtr_(documentManager)
    , preprocessor_(preprocessor)
    , searchThreadWorker_(searchThreadWorker)
{
    izenelib::util::CpuInfo::InitCpuTopologyInfo(s_cpu_topology_info);
    if (s_cpunum == 0)
    {
        s_cpunum = omp_get_num_procs();
        if (s_cpunum <= 0)
        {
            s_cpunum = 1;
        }
    }
}

void SearchThreadMaster::prepareThreadParams(
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

bool SearchThreadMaster::runThreadParams(
    std::vector<SearchThreadParam>& threadParams)
{
    if (threadParams.size() == 1)
        return runSingleThread_(threadParams.front());

    return runMultiThreads_(threadParams);
}

bool SearchThreadMaster::mergeThreadParams(
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

bool SearchThreadMaster::fetchSearchResult(
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
    
    std::size_t count = 0;
    if (offset < scoreItemQueue->size())
    {
        count = scoreItemQueue->size() - offset;
    }
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
    if (pSorter)
    {
        try
        {
            // all sorters will be the same after searching,
            // so we can just use any sorter.
            preprocessor_.fillSearchInfoWithSortPropertyData(pSorter.get(),
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

void SearchThreadMaster::getThreadInfo_(
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

bool SearchThreadMaster::runSingleThread_(
    SearchThreadParam& threadParam)
{
    bool result = false;

    try
    {
        result = searchThreadWorker_.search(threadParam);
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << e.what();
    }

    return result;
}

bool SearchThreadMaster::runMultiThreads_(
    std::vector<SearchThreadParam>& threadParams)
{
    const std::size_t threadNum = threadParams.size();

    if (!s_cpu_topology_info.cpu_topology_supported)
    {
        boost::detail::atomic_count finishedJobs(0);
        #pragma omp parallel for schedule(dynamic, 2)
        for (std::size_t i = 0; i < threadNum; ++i)
        {
            runSearchJob_(&threadParams[i], &finishedJobs);
        }
    }
    else
    {
        boost::detail::atomic_count finishedJobs(0);
        for (std::size_t i = 0; i < threadNum; ++i)
        {
            threadpool_.schedule(
                boost::bind(&SearchThreadMaster::runSearchJob_,
                            this, &threadParams[i], &finishedJobs));

        }
        threadpool_.wait(finishedJobs, threadNum);
    }

    return true;
}

void SearchThreadMaster::runSearchJob_(
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
        pParam->isSuccess = searchThreadWorker_.search(*pParam);
    }
    ++(*finishedJobs);
}
