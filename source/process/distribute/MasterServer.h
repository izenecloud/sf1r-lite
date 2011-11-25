/**
 * @file MasterServer.h
 * @author Zhongxia Li
 * @date Sep 1, 2011
 * @brief 
 */
#ifndef MASTER_SERVER_H_
#define MASTER_SERVER_H_

#include <net/aggregator/MasterServerBase.h>

#include <common/CollectionManager.h>
#include <controllers/CollectionHandler.h>

using namespace net::aggregator;

namespace sf1r
{

class MasterServer : public MasterServerBase
{
public:
    static MasterServer* get()
    {
        return izenelib::util::Singleton<MasterServer>::get();
    }

protected:
    virtual bool notify(NotifyMSG& msg)
    {
        if (msg.method == "clear_search_cache")
        {
            CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(msg.identity);
            if (collectionHandler)
            {
                collectionHandler->indexSearchService_->OnUpdateSearchCache();
                return true;
            }
            else
            {
                cout << "#[MasterServer] notify error, no collectionHandler found for " + msg.identity;
                return false;
            }
        }
        return false;
    }
};

}

#endif /* MASTER_SERVER_H_ */
