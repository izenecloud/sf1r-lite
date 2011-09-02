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
#include <common/Utilities.h>
#include <common/JobScheduler.h>
#include <controllers/CollectionHandler.h>
#include <bundles/index/IndexSearchService.h>
#include <bundles/index/IndexTaskService.h>

using namespace net::aggregator;

namespace sf1r
{

class KeywordSearchActionItem;
class KeywordSearchResult;

class WorkerServer : public JobWorker<WorkerServer>
{
private:
    // A coming request should be target at a specified collection or a bundle which not limited
    // to any collection, set to corresponding worker service before handling request.
    boost::shared_ptr<WorkerService> workerService_;

    // worker service for querylog bundle
    boost::shared_ptr<WorkerService> queryLogWorkerService_;

public:
//    static WorkerServer* get()
//    {
//        return izenelib::util::Singleton<WorkerServer>::get();
//    }

    WorkerServer(const std::string& host, uint16_t port, unsigned int threadNum)
    : JobWorker<WorkerServer>(host, port, threadNum)
    {
        queryLogWorkerService_.reset(new WorkerService());
    }

    void setQueryLogSearchService(QueryLogSearchService* queryLogSearchService)
    {
        queryLogWorkerService_->queryLogSearchService_ = queryLogSearchService;
    }

public:

    /**
     * Pre-process before dispatch (handle) a received request,
     * identity is info such as collection, bundle name.
     */
    virtual bool preHandle(const std::string& identity, std::string& error)
    {
        if (debug_)
            cout << "#[WorkerServer::preHandle] identity : " << identity << endl;

        identity_ = identity;

        std::string identityLow = sf1r::Utilities::toLower(identity);
        if (identityLow == "querylog")
        {
            if (sf1r::SF1Config::get()->checkQueryLogWorkerService())
            {
                workerService_ = queryLogWorkerService_;
                return true;
            }
            else
            {
                error = "QueryLog worker service is not enabled!";
                return false;
            }
        }

        if (sf1r::SF1Config::get()->checkCollectionWorkerService(identity))
        {
            CollectionHandler* collectionHandler_ = CollectionManager::get()->findHandler(identity);
            if (!collectionHandler_)
            {
                error = "No collectionHandler found for " + identity;
                return false;
            }
            workerService_ = collectionHandler_->indexSearchService_->workerService_;
        }
        else
        {
            error = "Worker service is not enabled for " + identity;
            return false;
        }

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
        ADD_WORKER_HANDLER( getSimilarDocIdList )
        ADD_WORKER_HANDLER( clickGroupLabel )
        ADD_WORKER_HANDLER( visitDoc )
        ADD_WORKER_HANDLER( index )

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

    bool getSimilarDocIdList(JobRequest& req)
    {
        WORKER_HANDLE_REQUEST_2_1(req, uint64_t, uint32_t, workerService_->getSimilarDocIdList, SimilarDocIdListType)
        return true;
    }

    bool clickGroupLabel(JobRequest& req)
    {
        WORKER_HANDLE_REQUEST_1_1(req, ClickGroupLabelActionItem, bool, workerService_, clickGroupLabel)
        return true;
    }

    bool visitDoc(JobRequest& req)
    {
        WORKER_HANDLE_REQUEST_1_1(req, uint32_t, bool, workerService_, visitDoc)
        return true;
    }

    bool index(JobRequest& req)
    {
        WORKER_HANDLE_REQUEST_1_1_(req, unsigned int, index, bool)
        return true;
    }

    /** @} */

private:
    bool index(const unsigned int& numdoc, bool& ret)
    {
        std::string bundleName = "IndexBundle-" + identity_;
        IndexTaskService* indexService = static_cast<IndexTaskService*>(
                                             CollectionManager::get()->getOSGILauncher().getService(bundleName, "IndexTaskService"));
        if (!indexService)
        {
            ret = false;
        }
        else
        {
            ret = true;
            task_type task = boost::bind(&IndexTaskService::buildCollection, indexService, numdoc);
            JobScheduler::get()->addTask(task);
        }

        return ret;
    }

private:
    std::string identity_;
};


}

#endif /* WORKER_SERVER_H_ */
