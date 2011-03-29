#include "CollectionManager.h"
#include "CollectionMeta.h"

namespace sf1r
{

CollectionHandler* CollectionManager::kEmptyHandler_ = 0;

std::auto_ptr<CollectionHandler> 
CollectionManager::startCollection(const string& collectionName, const std::string& configFileName)
{
    std::auto_ptr<CollectionHandler> collectionHandler(new CollectionHandler(collectionName));

    boost::shared_ptr<IndexBundleConfiguration> indexBundleConfig(new IndexBundleConfiguration(collectionName));
    boost::shared_ptr<MiningBundleConfiguration> miningBundleConfig(new MiningBundleConfiguration(collectionName));

    CollectionMeta collectionMeta;
    collectionMeta.indexBundleConfig_ = indexBundleConfig;
    collectionMeta.miningBundleConfig_ = miningBundleConfig;

    CollectionConfig::get()->parseConfigFile(collectionName, configFileName, collectionMeta);

    indexBundleConfig->setSchema(collectionMeta.getDocumentSchema());

    ///createIndexBundle
    std::string bundleName = collectionName + "-index";
    IndexSearchService* indexSearchService = static_cast<IndexSearchService*>(osgiLauncher_.getService(bundleName, "IndexSearchService"));
    collectionHandler->registerService(indexSearchService);
    osgiLauncher_.start(indexBundleConfig);
    collectionHandler->setBundleSchema(indexBundleConfig->schema_);

    ///createMiningBundle
    bundleName = collectionName + "-mining";
    MiningSearchService* miningSearchService = static_cast<MiningSearchService*>(osgiLauncher_.getService(bundleName, "MiningSearchService"));
    collectionHandler->registerService(miningSearchService);
    osgiLauncher_.start(miningBundleConfig);

    // insert first, then assign to ensure exception safe
    std::pair<handler_map_type::iterator, bool> insertResult =
        collectionHandlers_.insert(std::make_pair(collectionName, kEmptyHandler_));
    insertResult.first->second = collectionHandler.release();

    return collectionHandler;
}

void CollectionManager::stopCollection(const std::string& collectionName)
{
    std::string bundleName = collectionName + "-index";
    //boost::shared_ptr<BundleConfiguration> bundleConfigPtr = 
    //    osgiLauncher_->getBundleInfo(bundleName)->getBundleContext()->getBundleConfig();
    //config_ = dynamic_cast<IndexBundleConfiguration*>(bundleConfigPtr.get());

    osgiLauncher_.stopBundle(bundleName);
    bundleName = collectionName + "-mining";
    osgiLauncher_.stopBundle(bundleName);
}

void CollectionManager::deleteCollection(const std::string& collectionName)
{

}

CollectionHandler* CollectionManager::findHandler(const std::string& key) const
{
    typedef handler_map_type::const_iterator iterator;

    iterator findResult = collectionHandlers_.find(key);
    if (findResult != collectionHandlers_.end())
    {
        return findResult->second;
    }
    else
    {
        return kEmptyHandler_;
    }
}

}

