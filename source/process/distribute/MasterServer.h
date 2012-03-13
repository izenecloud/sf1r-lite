#ifndef PROCESS_DISTRIBUTE_MASTER_SERVER_H_
#define PROCESS_DISTRIBUTE_MASTER_SERVER_H_

#include <3rdparty/msgpack/rpc/server.h>

#include <common/ResultType.h>
#include <query-manager/ActionItem.h>
#include <aggregator-manager/MasterServerConnector.h>
#include <common/CollectionManager.h>
#include <controllers/CollectionHandler.h>
#include <index/IndexSearchService.h>

namespace sf1r
{

/**
 * Search/Recommend Master RPC Server
 */
class MasterServer : public msgpack::rpc::server::base
{
public:
    ~MasterServer()
    {
        stop();
    }

    void start(const std::string& host, uint16_t port, unsigned int threadnum=4)
    {
        instance.listen(host, port);
        instance.start(threadnum);
    }

    void stop()
    {
        if (instance.is_running())
        {
            instance.end();
            instance.join();
        }
    }

    void dispatch(msgpack::rpc::request req)
    {
        try
        {
            std::string method;
            req.method().convert(&method);

            if (method == MasterServerConnector::Methods_[MasterServerConnector::Method_getDocumentsByIds_])
            {
                getDocumentsByIds(req);
            }
        }
        catch (msgpack::type_error& e)
        {
            req.error(msgpack::rpc::ARGUMENT_ERROR);
            cout << "#[Master] notified, Argument error!"<<endl;
        }
        catch (std::exception& e)
        {
            cout << "#[Master] notified, "<<e.what()<<endl;
            req.error(std::string(e.what()));
        }

        // release collection lock
        if (curCollectionMutex_)
            curCollectionMutex_->unlock_shared();
    }

private:
    /**
     * @example rpc call
     *   GetDocumentsByIdsActionItem action;
     *   RawTextResultFromSIA resultItem;
     *   MasterServerConnector::get()->syncCall(MasterServerConnector::Method_getDocumentsByIds_, action, resultItem);
     */
    void getDocumentsByIds(msgpack::rpc::request& req)
    {
        msgpack::type::tuple<GetDocumentsByIdsActionItem> params;
        req.params().convert(&params);
        GetDocumentsByIdsActionItem action = params.get<0>();

        CollectionHandler* collectionHandler = getCollectionHandler(action.collectionName_);
        if (!collectionHandler)
        {
            std::string error = "No collectionHandler found for " + action.collectionName_;
            req.error(error);
            return;
        }

        RawTextResultFromSIA resultItem;
        if (!collectionHandler->indexSearchService_->getDocumentsByIds(action, resultItem))
        {
            req.error(resultItem.error_);
            return;
        }

        req.result(resultItem);
    }

private:
    CollectionHandler* getCollectionHandler(const std::string& collection)
    {
        //std::string collectionLow = boost::to_lower_copy(collection);
        curCollectionMutex_ = CollectionManager::get()->getCollectionMutex(collection);
        curCollectionMutex_->lock_shared();
        return CollectionManager::get()->findHandler(collection);
    }

private:
    CollectionManager::MutexType* curCollectionMutex_;
};

}

#endif /* PROCESS_DISTRIBUTE_MASTER_SERVER_H_ */
