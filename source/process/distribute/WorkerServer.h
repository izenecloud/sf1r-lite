/**
 * @file WorkerServer.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief
 */
#ifndef PROCESS_DISTRIBUTE_WORKER_SERVER_H_
#define PROCESS_DISTRIBUTE_WORKER_SERVER_H_

#include <aggregator-manager/SearchWorker.h>
#include <aggregator-manager/IndexWorker.h>

#include <net/aggregator/WorkerServerBase.h>
#include <util/singleton.h>

#include <common/CollectionManager.h>
#include <common/JobScheduler.h>
#include <controllers/CollectionHandler.h>
#include <bundles/index/IndexSearchService.h>
#include <bundles/index/IndexTaskService.h>

using namespace net::aggregator;

namespace sf1r
{

class KeywordSearchActionItem;
class KeywordSearchResult;

class WorkerServer : public WorkerServerBase<WorkerServer>
{
public:
    WorkerServer() {}

    static WorkerServer* get()
    {
        return izenelib::util::Singleton<WorkerServer>::get();
    }

    void init(const std::string& host, uint16_t port, unsigned int threadNum, bool debug=false)
    {
        WorkerServerBase<WorkerServer>::init(host, port, threadNum, debug);
    }

public:

    /**
     * Pre-process before dispatch (handle) a received request,
     * identity is info such as collection, bundle name.
     */
    virtual bool preprocess(const std::string& identity, std::string& error)
    {
        if (debug_)
            cout << "#[WorkerServer::preHandle] identity : " << identity << endl;

        identity_ = identity;

        std::string identityLow = boost::to_lower_copy(identity);
        if (sf1r::SF1Config::get()->checkSearchWorker(identity))
        {
            // A collection may be stopped with all resources released,
            // so we should lock current the current collection during operations on it,
            // and release lock when each operation finished (post-process).
            curCollectionMutex_ = CollectionManager::get()->getCollectionMutex(identity);
            curCollectionMutex_->lock_shared();
            collectionHandler_ = CollectionManager::get()->findHandler(identity);

            if (!collectionHandler_)
            {
                error = "No collectionHandler found for " + identity;
                std::cout << error << std::endl;
                curCollectionMutex_->unlock_shared();
                return false;
            }

            searchWorker_ = collectionHandler_->indexSearchService_->searchWorker_;
        }
        else
        {
            error = "Worker service is not enabled for " + identity;
            std::cout << error << std::endl;
            return false;
        }

        return true;
    }

    virtual void postprocess()
    {
        if (curCollectionMutex_)
            curCollectionMutex_->unlock_shared();
    }

    /**
     * Handlers for handling received requests.
     */
    virtual void addServerHandlers()
    {
        ADD_WORKER_HANDLER_LIST_BEGIN( WorkerServer )

        ADD_WORKER_HANDLER( getDistSearchInfo )
        ADD_WORKER_HANDLER( getDistSearchResult )
        ADD_WORKER_HANDLER( getSummaryResult )
        ADD_WORKER_HANDLER( getSummaryMiningResult )
        ADD_WORKER_HANDLER( getDocumentsByIds )
        ADD_WORKER_HANDLER( getInternalDocumentId )
        ADD_WORKER_HANDLER( getSimilarDocIdList )
        ADD_WORKER_HANDLER( clickGroupLabel )
        ADD_WORKER_HANDLER( visitDoc )
        ADD_WORKER_HANDLER( index )

        ADD_WORKER_HANDLER_LIST_END()
    }

public:
    /**
     * Publish worker services to remote procedure (as remote server)
     * @{
     */

    void getDistSearchInfo(request_t& req)
    {
        WORKER_HANDLE_1_1(req, KeywordSearchActionItem, searchWorker_->getDistSearchInfo, DistKeywordSearchInfo)

    }

    void getDistSearchResult(request_t& req)
    {
        WORKER_HANDLE_1_1(req, KeywordSearchActionItem, searchWorker_->getDistSearchResult, DistKeywordSearchResult)
    }

    void getSummaryResult(request_t& req)
    {
        WORKER_HANDLE_1_1(req, KeywordSearchActionItem, searchWorker_->getSummaryResult, KeywordSearchResult)
    }

    void getSummaryMiningResult(request_t& req)
    {
        WORKER_HANDLE_1_1(req, KeywordSearchActionItem, searchWorker_->getSummaryMiningResult, KeywordSearchResult)
    }

    void getDocumentsByIds(request_t& req)
    {
        WORKER_HANDLE_1_1(req, GetDocumentsByIdsActionItem, searchWorker_->getDocumentsByIds, RawTextResultFromSIA)
    }

    void getInternalDocumentId(request_t& req)
    {
        WORKER_HANDLE_1_1(req, izenelib::util::UString, searchWorker_->getInternalDocumentId, uint64_t)
    }

    void getSimilarDocIdList(request_t& req)
    {
        WORKER_HANDLE_2_1(req, uint64_t, uint32_t, searchWorker_->getSimilarDocIdList, SimilarDocIdListType)
    }

    void clickGroupLabel(request_t& req)
    {
        WORKER_HANDLE_1_1(req, ClickGroupLabelActionItem, searchWorker_->clickGroupLabel, bool)
    }

    void visitDoc(request_t& req)
    {
        WORKER_HANDLE_1_1(req, uint32_t, searchWorker_->visitDoc, bool)
    }

    void index(request_t& req)
    {
        boost::shared_ptr<IndexWorker> indexWorker = collectionHandler_->indexTaskService_->indexWorker_;
        WORKER_HANDLE_1_1(req, unsigned int, indexWorker->index, bool)
    }

    /** @} */

private:
    // A coming request is targeted at a specified collection or a bundle,
    // so find corresponding collectionHandler before handling the request.
    CollectionManager::MutexType* curCollectionMutex_;
    CollectionHandler* collectionHandler_;
    boost::shared_ptr<SearchWorker> searchWorker_;

    std::string identity_;
};


}

#endif /* PROCESS_DISTRIBUTE_WORKER_SERVER_H_ */
