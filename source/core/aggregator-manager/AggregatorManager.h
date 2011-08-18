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

class AggregatorManager : public JobAggregator<AggregatorManager>
{

public:
    /**
     * Interface which encapsulate calls to local service directly
     */
    template <typename RequestType, typename ResultType>
    bool get_local_result(
            const std::string& func,
            const RequestType& request,
            ResultType& result,
            std::string& error)
    {
        if (localWorkerService_)
        {
            return localWorkerService_->call(func, request, result, error);
        }
        else
        {
            error = "No local worker service.";
            return false;
        }
    }


public:
    /**
     * TODO, overload join_impl() to aggregate result(s) got from server node(s).
     *
     * @{
     */

    void join_impl(const std::string& func,KeywordPreSearchResult& result, const std::vector<std::pair<workerid_t, KeywordPreSearchResult> >& resultList)
    {
        if (func == "getPreSearchResult")
        {
            aggregatePreSearchResult(result, resultList);
        }
    }


    void join_impl(const std::string& func,KeywordRealSearchResult& result, const std::vector<std::pair<workerid_t, KeywordRealSearchResult> >& resultList)
    {
        if (func == "getSearchResult")
        {
        	aggregateSearchResult(result, resultList);
        }
    }

    void join_impl(const std::string& func, KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList)
    {
    	if (func == "getSummaryResult")
        {
        	aggregateSummaryResult(result, resultList);
        }
    }

    void join_impl(const std::string& func, RawTextResultFromSIA& result, const std::vector<std::pair<workerid_t, RawTextResultFromSIA> >& resultList)
    {
    	if (func == "getDocumentsByIds")
    	{
    		aggregateDocumentsResult(result, resultList);
    	}
    }

    void join_impl(const std::string& func, bool& ret, const std::vector<std::pair<workerid_t, bool> >& resultList)
    {
        if (func == "clickGroupLabel")
        {
            // nothing
        }
    }

    /** @}*/

public:
    void aggregatePreSearchResult(KeywordPreSearchResult& result, const std::vector<std::pair<workerid_t, KeywordPreSearchResult> >& resultList)
    {

    }

    void aggregateSearchResult(KeywordRealSearchResult& result, const std::vector<std::pair<workerid_t, KeywordRealSearchResult> >& resultList);

    void aggregateSummaryResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList);

    void aggregateDocumentsResult(RawTextResultFromSIA& result, const std::vector<std::pair<workerid_t, RawTextResultFromSIA> >& resultList);


public:
    void setLocalWorkerService(const boost::shared_ptr<WorkerService>& localWorkerService)
    {
        localWorkerService_ = localWorkerService;
    }

    void SetMiningManager(const boost::shared_ptr<MiningManager>& mining_manager)
    {
    	mining_manager_ = mining_manager;
    }

    /**
     * Split data by workerid to sub data for requesting different workers.
     * @param result [IN]
     * @param resultList [OUT]
     */
    void splitResultByWorkerid(const KeywordSearchResult& result, std::map<workerid_t, boost::shared_ptr<KeywordSearchResult> >& resultMap);

    /**
     *
     * @param actionItem [IN]
     * @param actionItemMap [OUT]
     * @return <false, workerid> if only one worker, <false, -1> if no workerid(request all), else <true, -1>.
     */
    std::pair<bool, workerid_t> splitGetDocsActionItemByWorkerid(
            const GetDocumentsByIdsActionItem& actionItem,
            std::map<workerid_t, boost::shared_ptr<GetDocumentsByIdsActionItem> >& actionItemMap);

    /**
     * Merge data got from different workers.
     * @param result [OUT]
     * @param resultList [IN]
     */
    void mergeSummaryResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, boost::shared_ptr<KeywordSearchResult> > >& resultList);

    void mergeMiningResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, boost::shared_ptr<KeywordSearchResult> > >& resultList);
    
    

private:
    boost::shared_ptr<WorkerService> localWorkerService_;
    
    boost::shared_ptr<MiningManager> mining_manager_;
};


} // end - namespace

#endif /* AGGREGATOR_MANAGER_H_ */
