/**
 * @file WorkerServer.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief 
 */
#ifndef WORKER_SERVER_H_
#define WORKER_SERVER_H_

#include <net/aggregator/JobWorker.h>
#include "WorkerService.h"

//#include <util/singleton.h>

#include <common/CollectionManager.h>
#include <controllers/CollectionHandler.h>
#include <bundles/index/IndexSearchService.h>

using namespace net::aggregator;

namespace sf1r
{

class KeywordSearchActionItem;
class KeywordSearchResult;

class WorkerServer : public JobWorker<WorkerServer>
{
private:
    boost::shared_ptr<WorkerService> workerService_;

public:
//    static WorkerServer* get()
//    {
//        return izenelib::util::Singleton<WorkerServer>::get();
//    }

    WorkerServer(const std::string& host, uint16_t port, unsigned int threadNum)
    : JobWorker<WorkerServer>(host, port, threadNum)
    {}

public:

    /**
     * Pre-process before dispatch (handle) a received request,
     * identity is info such as collection, bundle name.
     */
    virtual bool preHandle(const std::string& identity, std::string& error)
    {
        if (debug_)
            cout << "#[WorkerServer::preHandle] identity : " << identity << endl;

        // todo, check if bundle worker service enabled(queryLog)
        if (!sf1r::SF1Config::get()->checkCollectionWorkerServer(identity))
        {
            error = "Worker service is not enabled for " + identity;
            return false;
        }

        // todo, check if identity is bundle name(queryLog)
        CollectionHandler* collectionHandler_ = CollectionManager::get()->findHandler(identity);
        if (!collectionHandler_)
        {
            error = "No collection found for " + identity;
            return false;
        }

        workerService_ = collectionHandler_->indexSearchService_->workerService_;
        return true;
    }

    /**
     * Handlers for processing received remote requests.
     */
    virtual void addHandlers()
    {
        ADD_WORKER_HANDLER_LIST_BEGIN( WorkerServer )

        ADD_WORKER_HANDLER( getDistSearchInfo )
        ADD_WORKER_HANDLER( getDistSearchResult )
        ADD_WORKER_HANDLER( getSummaryMiningResult )
        ADD_WORKER_HANDLER( getDocumentsByIds )
        ADD_WORKER_HANDLER( getInternalDocumentId )
        ADD_WORKER_HANDLER( clickGroupLabel )

        ADD_WORKER_HANDLER_LIST_END()
    }

    /**
     * Publish worker services to remote procedure (as remote server)
     * @{
     */

    bool getDistSearchInfo(JobRequest& req)
    {
        WORKER_HANDLE_REQUEST_1_1(req, KeywordSearchActionItem, DistKeywordSearchInfo, workerService_, getDistSearchInfo)
        return true;
    }

    bool getDistSearchResult(JobRequest& req)
    {
        WORKER_HANDLE_REQUEST_1_1(req, KeywordSearchActionItem, DistKeywordSearchResult, workerService_, getDistSearchResult)
        return true;
    }

    bool getSummaryMiningResult(JobRequest& req)
    {
        WORKER_HANDLE_REQUEST_1_1(req, KeywordSearchActionItem, KeywordSearchResult, workerService_, getSummaryMiningResult)
        return true;
    }

    bool getDocumentsByIds(JobRequest& req)
    {
        WORKER_HANDLE_REQUEST_1_1(req, GetDocumentsByIdsActionItem, RawTextResultFromSIA, workerService_, getDocumentsByIds)
    	return true;
    }

    bool getInternalDocumentId(JobRequest& req)
    {
        WORKER_HANDLE_REQUEST_1_1(req, izenelib::util::UString, uint64_t, workerService_, getInternalDocumentId)
        return true;
    }

    bool clickGroupLabel(JobRequest& req)
    {
        WORKER_HANDLE_REQUEST_1_1(req, ClickGroupLabelActionItem, bool, workerService_, clickGroupLabel)
        return true;
    }

    /** @} */
};


}

#endif /* WORKER_SERVER_H_ */
