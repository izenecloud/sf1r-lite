#include "CollectionManager.h"
#include "CollectionMeta.h"
#include <controllers/CollectionHandler.h>

#include <bundles/index/IndexBundleActivator.h>
#include <bundles/product/ProductBundleActivator.h>
#include <bundles/mining/MiningBundleActivator.h>
#include <bundles/recommend/RecommendBundleActivator.h>

#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

namespace sf1r
{

CollectionHandler* CollectionManager::kEmptyHandler_ = 0;

CollectionManager::~CollectionManager()
{
    for(handler_const_iterator iter = handlerBegin(); iter != handlerEnd(); iter++)
    {
         delete iter->second;
    }

    std::map<std::string, MutexType*>::const_iterator mutexIter;
    for(mutexIter = mutexMap_.begin(); mutexIter != mutexMap_.end(); mutexIter++)
    {
        delete mutexIter->second;
    }
}

CollectionHandler* CollectionManager::startCollection(const string& collectionName, const std::string& configFileName, bool fixBasePath)
{
    ScopedWriteLock lock(*getCollectionMutex(collectionName));

    CollectionHandler* collectionHandler = findHandler(collectionName);
    if(collectionHandler != NULL)
        return collectionHandler;
    else
        collectionHandler = new CollectionHandler(collectionName);

    boost::shared_ptr<IndexBundleConfiguration> indexBundleConfig(new IndexBundleConfiguration(collectionName));
    boost::shared_ptr<ProductBundleConfiguration> productBundleConfig(new ProductBundleConfiguration(collectionName));
    boost::shared_ptr<MiningBundleConfiguration> miningBundleConfig(new MiningBundleConfiguration(collectionName));
    boost::shared_ptr<RecommendBundleConfiguration> recommendBundleConfig(new RecommendBundleConfiguration(collectionName));

    CollectionMeta collectionMeta;
    collectionMeta.indexBundleConfig_ = indexBundleConfig;
    collectionMeta.productBundleConfig_ = productBundleConfig;
    collectionMeta.miningBundleConfig_ = miningBundleConfig;
    collectionMeta.recommendBundleConfig_ = recommendBundleConfig;

    if (!CollectionConfig::get()->parseConfigFile(collectionName, configFileName, collectionMeta))
    {
        throw XmlConfigParserException("error in parsing " + configFileName);
    }

    SF1Config::CollectionMetaMap& collectionMetaMap = SF1Config::get()->mutableCollectionMetaMap();
    SF1Config::CollectionMetaMap::value_type mapValue(collectionMeta.getName(), collectionMeta);
    if (!collectionMetaMap.insert(mapValue).second)
    {
        throw XmlConfigParserException("duplicated CollectionMeta name " + collectionMeta.getName());
    }

    if (fixBasePath)
    {
        bfs::path basePath(indexBundleConfig->collPath_.getBasePath());
        if (basePath.filename().string() == ".")
            basePath = basePath.parent_path().parent_path();
        else
            basePath = basePath.parent_path();
        indexBundleConfig->collPath_.resetBasePath((basePath/collectionName).string());
        productBundleConfig->collPath_ =  indexBundleConfig->collPath_;
        miningBundleConfig->collPath_ =  indexBundleConfig->collPath_;
        recommendBundleConfig->collPath_ =  indexBundleConfig->collPath_;
    }

    ///createIndexBundle
    if (indexBundleConfig->isSchemaEnable_)
    {
        std::string bundleName = "IndexBundle-" + collectionName;
        DYNAMIC_REGISTER_BUNDLE_ACTIVATOR_CLASS(bundleName, IndexBundleActivator);
        osgiLauncher_.start(indexBundleConfig);
        IndexSearchService* indexSearchService = static_cast<IndexSearchService*>(osgiLauncher_.getService(bundleName, "IndexSearchService"));
        collectionHandler->registerService(indexSearchService);
        IndexTaskService* indexTaskService = static_cast<IndexTaskService*>(osgiLauncher_.getService(bundleName, "IndexTaskService"));
        collectionHandler->registerService(indexTaskService);
        collectionHandler->setBundleSchema(indexBundleConfig->indexSchema_);
    }

    ///createProductBundle
    if (productBundleConfig->mode_>0)
    {
        std::string bundleName = "ProductBundle-" + collectionName;
        DYNAMIC_REGISTER_BUNDLE_ACTIVATOR_CLASS(bundleName, ProductBundleActivator);
        osgiLauncher_.start(productBundleConfig);
        ProductSearchService* productSearchService = static_cast<ProductSearchService*>(osgiLauncher_.getService(bundleName, "ProductSearchService"));
        collectionHandler->registerService(productSearchService);
        ProductTaskService* productTaskService = static_cast<ProductTaskService*>(osgiLauncher_.getService(bundleName, "ProductTaskService"));
        collectionHandler->registerService(productTaskService);
    }

    ///createMiningBundle
    if (miningBundleConfig->isSchemaEnable_)
    {
        std::string bundleName = "MiningBundle-" + collectionName;
        DYNAMIC_REGISTER_BUNDLE_ACTIVATOR_CLASS(bundleName, MiningBundleActivator);
        osgiLauncher_.start(miningBundleConfig);
        MiningSearchService* miningSearchService = static_cast<MiningSearchService*>(osgiLauncher_.getService(bundleName, "MiningSearchService"));
        collectionHandler->registerService(miningSearchService);
        collectionHandler->setBundleSchema(miningBundleConfig->mining_schema_);
    }

    ///createRecommendBundle
    if (recommendBundleConfig->isSchemaEnable_)
    {
        std::string bundleName = "RecommendBundle-" + collectionName;
        DYNAMIC_REGISTER_BUNDLE_ACTIVATOR_CLASS(bundleName, RecommendBundleActivator);
        osgiLauncher_.start(recommendBundleConfig);
        RecommendTaskService* recommendTaskService = static_cast<RecommendTaskService*>(osgiLauncher_.getService(bundleName, "RecommendTaskService"));
        collectionHandler->registerService(recommendTaskService);
        RecommendSearchService* recommendSearchService = static_cast<RecommendSearchService*>(osgiLauncher_.getService(bundleName, "RecommendSearchService"));
        collectionHandler->registerService(recommendSearchService);
        collectionHandler->setBundleSchema(recommendBundleConfig->recommendSchema_);
    }

    // insert first, then assign to ensure exception safe
    std::pair<handler_map_type::iterator, bool> insertResult =
        collectionHandlers_.insert(std::make_pair(collectionName, kEmptyHandler_));
    insertResult.first->second = collectionHandler;

    return collectionHandler;
}

void CollectionManager::stopCollection(const std::string& collectionName)
{
    ScopedWriteLock lock(*getCollectionMutex(collectionName));

    JobScheduler::get()->removeTask(collectionName);

    handler_const_iterator iter;
    if((iter = collectionHandlers_.find(collectionName)) != collectionHandlers_.end())
    {
        delete iter->second;
        collectionHandlers_.erase(collectionName);
    }

    std::string bundleName = "IndexBundle-" + collectionName;
    //boost::shared_ptr<BundleConfiguration> bundleConfigPtr =
    //    osgiLauncher_->getBundleInfo(bundleName)->getBundleContext()->getBundleConfig();
    //config_ = dynamic_cast<IndexBundleConfiguration*>(bundleConfigPtr.get());
    BundleInfoBase* bundleInfo = osgiLauncher_.getRegistry().getBundleInfo(bundleName);
    if(bundleInfo)
    {
        osgiLauncher_.stopBundle(bundleName);
    }

    bundleName = "MiningBundle-" + collectionName;
    bundleInfo = osgiLauncher_.getRegistry().getBundleInfo(bundleName);
    if(bundleInfo)
    {
        osgiLauncher_.stopBundle(bundleName);
    }

    bundleName = "ProductBundle-" + collectionName;
    bundleInfo = osgiLauncher_.getRegistry().getBundleInfo(bundleName);
    if(bundleInfo)
    {
        osgiLauncher_.stopBundle(bundleName);
    }

    bundleName = "RecommendBundle-" + collectionName;
    bundleInfo = osgiLauncher_.getRegistry().getBundleInfo(bundleName);
    if(bundleInfo)
    {
        osgiLauncher_.stopBundle(bundleName);
    }

    SF1Config::CollectionMetaMap& collectionMetaMap = SF1Config::get()->mutableCollectionMetaMap();
    SF1Config::CollectionMetaMap::iterator findIt = collectionMetaMap.find(collectionName);
    if (findIt != collectionMetaMap.end())
    {
        collectionMetaMap.erase(findIt);
    }
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

CollectionManager::handler_const_iterator CollectionManager::handlerBegin() const
{
    return collectionHandlers_.begin();
}

CollectionManager::handler_const_iterator CollectionManager::handlerEnd() const
{
    return collectionHandlers_.end();
}

}
