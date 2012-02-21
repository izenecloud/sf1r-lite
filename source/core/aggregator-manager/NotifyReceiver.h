/**
 * @file NotifyReceiver.h
 * @author Zhongxia Li
 * @date Sep 1, 2011
 * @brief
 */
#ifndef AGGREGATOR_NOTIFY_RECEIVER_H_
#define AGGREGATOR_NOTIFY_RECEIVER_H_

#include "Notifier.h"

#include <common/CollectionManager.h>
#include <controllers/CollectionHandler.h>

#include <index/IndexSearchService.h>

using namespace net::aggregator;

namespace sf1r
{

class NotifyRecevier : public msgpack::rpc::server::base
{
public:
    static NotifyRecevier* get()
    {
        return izenelib::util::Singleton<NotifyRecevier>::get();
    }

    void start(const std::string& host, uint16_t port, unsigned int threadnum = 4)
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

            msgpack::type::tuple<NotifyMSG> params;
            req.params().convert(&params);
            NotifyMSG msg = params.get<0>();

            if (!notify(msg))
            {
                cout << "#[Master] notified, "<<msg.error<<endl;
                req.error(msg.error);
            }
        }
        catch (const msgpack::type_error& e)
        {
            req.error(msgpack::rpc::ARGUMENT_ERROR);
            cout << "#[Master] notified, Argument error!" << endl;
            return;
        }
        catch (const std::exception& e)
        {
            cout << "#[Master] notified, " << e.what() << endl;
            req.error(std::string(e.what()));
            return;
        }
    }

protected:
    bool notify(NotifyMSG& msg)
    {
        if (msg.method == "clear_search_cache")
        {
            CollectionManager::MutexType* mutex = CollectionManager::get()->getCollectionMutex(msg.identity);
            CollectionManager::ScopedReadLock lock(*mutex);
            CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(msg.identity);
            if (collectionHandler)
            {
                collectionHandler->indexSearchService_->OnUpdateSearchCache();
                return true;
            }
            else
            {
                cout << "[NotifyRecevier] notify error, no collectionHandler found for " + msg.identity;
                return false;
            }
        }
        return false;
    }
};

}

#endif /* AGGREGATOR_NOTIFY_RECEIVER_H_ */
