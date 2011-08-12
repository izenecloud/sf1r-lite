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
#include <controllers/Sf1Controller.h>

#include <util/singleton.h>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include <memory> // for std::auto_ptr
#include <string>

namespace sf1r
{

class CollectionManager
{
public:
    typedef boost::unordered_map<std::string, CollectionHandler*> handler_map_type;
    typedef boost::unordered_map<std::string, CollectionHandler*>::const_iterator handler_const_iterator;
    static CollectionManager* get()
    {
        return ::izenelib::util::Singleton<CollectionManager>::get();
    }

    OSGILauncher& getOSGILauncher()
    {
        return this->osgiLauncher_;
    }

    std::auto_ptr<CollectionHandler> 
    startCollection(const std::string& collectionName, const std::string & configFileName);

    void stopCollection(const std::string& collectionName);

    void deleteCollection(const std::string& collectionName);

    CollectionHandler* findHandler(const std::string& key) const;
    
    handler_const_iterator handlerBegin() const;
    handler_const_iterator handlerEnd() const;

private:
    OSGILauncher osgiLauncher_;

    handler_map_type collectionHandlers_;

    static CollectionHandler* kEmptyHandler_;
};

}
#endif

