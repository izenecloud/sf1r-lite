#ifndef PROCESS_DISTRIBUTE_MASTER_SERVER_H_
#define PROCESS_DISTRIBUTE_MASTER_SERVER_H_

#include <util/singleton.h>
#include <3rdparty/msgpack/rpc/server.h>

#include <common/ResultType.h>
#include <query-manager/ActionItem.h>
#include <aggregator-manager/MasterServerConnector.h>

namespace sf1r
{

/**
 * Search/Recommend Master RPC Server
 */
class MasterServer : public msgpack::rpc::server::base
{
public:
    static MasterServer* get()
    {
        return izenelib::util::Singleton<MasterServer>::get();
    }

    void start(const std::string& host, uint16_t port, unsigned int threadnum=4)
    {
        instance.listen(host, port);
        instance.start(threadnum);
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
            return;
        }
        catch (std::exception& e)
        {
            cout << "#[Master] notified, "<<e.what()<<endl;
            req.error(std::string(e.what()));
            return;
        }
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
        collectionHandler->indexSearchService_->getDocumentsByIds(action, resultItem);
        req.result(resultItem);
    }

private:
    CollectionHandler* getCollectionHandler(const std::string& collection)
    {
        //std::string collectionLow = boost::to_lower_copy(collection);
        CollectionManager::MutexType* mutex = CollectionManager::get()->getCollectionMutex(collection);
        CollectionManager::ScopedReadLock lock(*mutex);
        return CollectionManager::get()->findHandler(collection);
    }
};

}

#endif /* PROCESS_DISTRIBUTE_MASTER_SERVER_H_ */
