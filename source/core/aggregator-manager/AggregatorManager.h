/**
 * @file AggregatorManager.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief 
 */
#ifndef AGGREGATOR_MANAGER_H_
#define AGGREGATOR_MANAGER_H_

#include <net/aggregator/JobInfo.h>
#include <net/aggregator/JobAggregator.h>
#include <net/aggregator/AggregatorConfig.h>

#include "WorkerService.h"

using namespace net::aggregator;

namespace sf1r
{

class KeywordSearchResult;
class MiningManager;

typedef WorkerCaller<WorkerService> LocalWorkerCaller;

class AggregatorManager : public JobAggregator<AggregatorManager, LocalWorkerCaller>
{
public:
    AggregatorManager() : TOP_K_NUM(4000) {}

public:
    void initLocalWorkerCaller(const boost::shared_ptr<WorkerService> localWorkerService)
    {
        localWorkerCaller_.reset(new LocalWorkerCaller);
        localWorkerCaller_->setInvoker(localWorkerService.get());

        ADD_WORKER_CALLER_METHOD(LocalWorkerCaller, localWorkerCaller_, WorkerService, getDistSearchInfo);
        ADD_WORKER_CALLER_METHOD(LocalWorkerCaller, localWorkerCaller_, WorkerService, getDistSearchResult);
        ADD_WORKER_CALLER_METHOD(LocalWorkerCaller, localWorkerCaller_, WorkerService, getSummaryMiningResult);
        ADD_WORKER_CALLER_METHOD(LocalWorkerCaller, localWorkerCaller_, WorkerService, getDocumentsByIds);
        ADD_WORKER_CALLER_METHOD(LocalWorkerCaller, localWorkerCaller_, WorkerService, getInternalDocumentId);
        ADD_WORKER_CALLER_METHOD(LocalWorkerCaller, localWorkerCaller_, WorkerService, getSimilarDocIdList);
        ADD_WORKER_CALLER_METHOD(LocalWorkerCaller, localWorkerCaller_, WorkerService, clickGroupLabel);
        ADD_WORKER_CALLER_METHOD(LocalWorkerCaller, localWorkerCaller_, WorkerService, visitDoc);
        ADD_WORKER_CALLER_METHOD(LocalWorkerCaller, localWorkerCaller_, WorkerService, index);
    }

public:
    /**
     * TODO, overload aggregate() to aggregate result(s) got from server node(s).
     *
     * @{
     */

    bool aggregate(const std::string& func,DistKeywordSearchInfo& result, const std::vector<std::pair<workerid_t, DistKeywordSearchInfo> >& resultList)
    {
        if (func == "getDistSearchInfo")
        {
            aggregateDistSearchInfo(result, resultList);
            return true;
        }
        return false;
    }


    bool aggregate(const std::string& func,DistKeywordSearchResult& result, const std::vector<std::pair<workerid_t, DistKeywordSearchResult> >& resultList)
    {
        if (func == "getDistSearchResult")
        {
        	aggregateDistSearchResult(result, resultList);
        	return true;
        }
        return false;
    }

    bool aggregate(const std::string& func, KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList)
    {
    	if (func == "getSummaryMiningResult")
        {
        	aggregateSummaryMiningResult(result, resultList);
        	return true;
        }
    	return false;
    }

    bool aggregate(const std::string& func, RawTextResultFromSIA& result, const std::vector<std::pair<workerid_t, RawTextResultFromSIA> >& resultList)
    {
    	if (func == "getDocumentsByIds")
    	{
    		aggregateDocumentsResult(result, resultList);
    		return true;
    	}
    	return false;
    }

    bool aggregate(const std::string& func, uint64_t& result, const std::vector<std::pair<workerid_t, uint64_t> >& resultList)
    {
        if (func == "getInternalDocumentId")
        {
            aggregateInternalDocumentId(result, resultList);
            return true;
        }
        return false;
    }

    bool aggregate(const std::string& func, bool& ret, const std::vector<std::pair<workerid_t, bool> >& resultList)
    {
        if (func == "clickGroupLabel")
        {
            // xxx
            ret = true;
            return true;
        }
        return false;
    }

    /** @}*/

private:
    /// search
    void aggregateDistSearchInfo(DistKeywordSearchInfo& result, const std::vector<std::pair<workerid_t, DistKeywordSearchInfo> >& resultList);

    void aggregateDistSearchResult(DistKeywordSearchResult& result, const std::vector<std::pair<workerid_t, DistKeywordSearchResult> >& resultList);

    void aggregateSummaryMiningResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList);

    /// documents
    void aggregateSummaryResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList);

    void aggregateDocumentsResult(RawTextResultFromSIA& result, const std::vector<std::pair<workerid_t, RawTextResultFromSIA> >& resultList);

    void aggregateInternalDocumentId(uint64_t& result, const std::vector<std::pair<workerid_t, uint64_t> >& resultList);

    /// mining
    void aggregateMiningResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList);

    //void aggregateSimilarDocIdList(SimilarDocIdListType& result, const std::vector<std::pair<workerid_t, SimilarDocIdListType> >& resultList);


public:
    /**
     * Split data by workerid to sub data for requesting different workers.
     * @param result [IN]
     * @param resultList [OUT]
     * @return true if successfully splited, or false.
     */
    bool splitSearchResultByWorkerid(const KeywordSearchResult& result, std::map<workerid_t, boost::shared_ptr<KeywordSearchResult> >& resultMap);

    /**
     *
     * @param actionItem [IN]
     * @param actionItemMap [OUT]
     * @return true if successfully splited, or false.
     */
    bool splitGetDocsActionItemByWorkerid(
            const GetDocumentsByIdsActionItem& actionItem,
            std::map<workerid_t, boost::shared_ptr<GetDocumentsByIdsActionItem> >& actionItemMap);
    

private:
    boost::shared_ptr<MiningManager> miningManager_;
    int TOP_K_NUM;

    friend class IndexSearchService;
    friend class IndexBundleActivator;
};


} // end - namespace

#endif /* AGGREGATOR_MANAGER_H_ */
