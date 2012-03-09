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
#include <boost/thread.hpp>

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

    MutexType* getCollectionMutex(const std::string& collection);

    bool startCollection(
            const std::string& collectionName,
            const std::string& configFileName,
            bool fixBasePath = false);

    void stopCollection(const std::string& collectionName);

    void deleteCollection(const std::string& collectionName);

    CollectionHandler* findHandler(const std::string& key) const;
    
    handler_const_iterator handlerBegin() const;
    handler_const_iterator handlerEnd() const;

private:
    OSGILauncher osgiLauncher_;

    handler_map_type collectionHandlers_;

    boost::mutex mapMutex_;

    typedef std::map<std::string, MutexType*> CollectionMutexes;
    CollectionMutexes collectionMutexes_;

    friend class RemoteItemManagerTestFixture;
};

}
#endif

