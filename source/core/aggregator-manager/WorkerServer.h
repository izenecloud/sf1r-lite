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
     * interfaces for handling remote request
     * @{
     */

    virtual bool preHandle(const std::string& key)
    {
        //cout << "#[WorkerServer::preHandle] key : " << key<<endl;

        if (!sf1r::SF1Config::get()->checkCollectionWorkerServer(key))
        {
            return false;
        }

        CollectionHandler* collectionHandler_ = CollectionManager::get()->findHandler(key);
        if (!collectionHandler_)
        {
            return false;
        }

        workerService_ = collectionHandler_->indexSearchService_->workerService_;

        return true;
    }

    /*pure virtual*/
    void addHandlers()
    {
        ADD_WORKER_HANDLER_LIST_BEGIN( WorkerServer )

        ADD_WORKER_HANDLER( getSearchResult )
        ADD_WORKER_HANDLER( getSummaryResult )
        // todo, add more ...

        ADD_WORKER_HANDLER_LIST_END()
    }

    bool getSearchResult(JobRequest& req)
    {
        WORKER_HANDLE_REQUEST_1_1(req, KeywordSearchActionItem, KeywordSearchResult, workerService_, processGetSearchResult)
        return true;
    }

    bool getSummaryResult(JobRequest& req)
    {
        WORKER_HANDLE_REQUEST_1_1(req, KeywordSearchActionItem, KeywordSearchResult, workerService_, processGetSummaryResult)
        return true;
    }

    /** @}*/
};

}

#endif /* WORKER_SERVER_H_ */
