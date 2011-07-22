/**
 * @file WorkerService.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief Worker Service provides services of local SF1 server.
 * It can work as a RPC Server by calling startServer(host,port,threadnum), which provides remote services for Aggregator(client).
 */
#ifndef WORKER_SERVICE_H_
#define WORKER_SERVICE_H_

#include <net/aggregator/JobInfo.h>
#include <net/aggregator/JobWorker.h>
#include <net/aggregator/WorkerHandler.h>

#include <util/singleton.h>

#include <query-manager/ActionItem.h>
#include <common/ResultType.h>

#include <boost/shared_ptr.hpp>

using namespace net::aggregator;

namespace sf1r
{

class IndexSearchService;

class WorkerService : public JobWorker<WorkerService>
{
public:
//    static WorkerService* get()
//    {
//        return izenelib::util::Singleton<WorkerService>::get();
//    }

    WorkerService(IndexSearchService* indexSearchService)
    :indexSearchService_(indexSearchService)
    {
        BOOST_ASSERT(indexSearchService_);
    }

public:
    /**
     * interfaces for handling remote request
     * @{
     */

    /*pure virtual*/
    void addHandlers()
    {
        ADD_WORKER_HANDLER_LIST_BEGIN( WorkerService )

        ADD_WORKER_HANDLER( getSearchResult )
        ADD_WORKER_HANDLER( getSummaryResult )
        // todo, add more ...

        ADD_WORKER_HANDLER_LIST_END()
    }

    bool getSearchResult(JobRequest& req)
    {
        WORKER_HANDLE_1_1(req, KeywordSearchActionItem, processGetSearchResult, KeywordSearchResult)
        return true; // xxx
    }

    bool getSummaryResult(JobRequest& req)
    {
        // xxx
        return true;
    }

    /** @}*/

public:
    /**
     * interfaces for handling local request (call directly)
     * @{
     */

    bool call(
            const std::string& func,
            const KeywordSearchActionItem& request,
            KeywordSearchResult& result,
            std::string& error)
    {
        if (func == "getSearchResult")
        {
            return processGetSearchResult(request, result);
        }
        else if (func == "getSummaryResult")
        {
            return processGetSummaryResult(request, result);
        }
        else
        {
            error = "no method!";
        }

        return false;
    }

    /** @}*/

private:
    bool processGetSearchResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    bool processGetSummaryResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
    {
        return false;
    }


private:
    IndexSearchService* indexSearchService_;
};

}

#endif /* WORKER_SERVICE_H_ */
