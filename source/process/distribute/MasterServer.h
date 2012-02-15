#ifndef PROCESS_DISTRIBUTE_MASTER_SERVER_H_
#define PROCESS_DISTRIBUTE_MASTER_SERVER_H_

#include <util/singleton.h>
#include <3rdparty/msgpack/rpc/server.h>

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

    bool preprocess(const std::string& collection, std::string& error)
    {
        std::cout << "#[WorkerServer::preHandle] collection : " << collection << endl;

        //std::string collectionLow = boost::to_lower_copy(collection);
        CollectionManager::MutexType* mutex = CollectionManager::get()->getCollectionMutex(collection);
        CollectionManager::ScopedReadLock lock(*mutex);
        collectionHandler_ = CollectionManager::get()->findHandler(collection);

        if (!collectionHandler_)
        {
            error = "No collectionHandler found for " + collection;
            std::cout << error << std::endl;
            return false;
        }

        return true;
    }

    void dispatch(msgpack::rpc::request req)
    {
        try
        {
            std::string method;
            req.method().convert(&method);

            ;
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
    CollectionHandler* collectionHandler_;
};

}

#endif /* PROCESS_DISTRIBUTE_MASTER_SERVER_H_ */
