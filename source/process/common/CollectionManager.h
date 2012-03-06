/**
 * @file CollectionManager.h
 * @brief Manage collection bundles
 * @author Yingfeng Zhang
 * @date 2011-02-25
 */
 #ifndef COLLECTION_FACTORY_H
#define COLLECTION_FACTORY_H

#include "XmlConfigParser.h"
#include "OSGILauncher.h"

#include <util/singleton.h>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

#include <memory> // for std::auto_ptr
#include <string>

namespace sf1r
{
class CollectionHandler;
class CollectionManager
{
public:
    typedef boost::unordered_map<std::string, CollectionHandler*> handler_map_type;
    typedef boost::unordered_map<std::string, CollectionHandler*>::const_iterator handler_const_iterator;
    typedef boost::shared_mutex MutexType;
    typedef boost::shared_lock<MutexType> ScopedReadLock;
    typedef boost::unique_lock<MutexType> ScopedWriteLock;

    static CollectionManager* get()
    {
        return ::izenelib::util::Singleton<CollectionManager>::get();
    }

    ~CollectionManager();

    OSGILauncher& getOSGILauncher()
    {
        return this->osgiLauncher_;
    }

    MutexType* getCollectionMutex(const std::string& collection)
    {
        boost::mutex::scoped_lock lock(mu_);
        if(mutexMap_.find(collection) == mutexMap_.end())
        {
            MutexType* p = new MutexType();
            mutexMap_[collection] = p;
        }
        return mutexMap_[collection];
    }

    CollectionHandler* startCollection(const std::string& collectionName, const std::string & configFileName);

    void stopCollection(const std::string& collectionName);

    void deleteCollection(const std::string& collectionName);

    CollectionHandler* findHandler(const std::string& key) const;
    
    handler_const_iterator handlerBegin() const;
    handler_const_iterator handlerEnd() const;

private:
    OSGILauncher osgiLauncher_;

    handler_map_type collectionHandlers_;

    boost::mutex mu_;

    std::map<std::string, MutexType*> mutexMap_;

    static CollectionHandler* kEmptyHandler_;

    friend class RemoteItemManagerTestFixture;
};

}
#endif

