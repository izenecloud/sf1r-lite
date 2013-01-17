#include "CollectionManager.h"
#include "CollectionMeta.h"
#include <controllers/CollectionHandler.h>

#include <bundles/index/IndexBundleActivator.h>
#include <bundles/product/ProductBundleActivator.h>
#include <bundles/mining/MiningBundleActivator.h>
#include <bundles/recommend/RecommendBundleActivator.h>
#include <aggregator-manager/GetRecommendWorker.h>
#include <aggregator-manager/UpdateRecommendWorker.h>
#include <common/JobScheduler.h>
#include <license-manager/LicenseCustManager.h>
#include <license-manager/LicenseTool.h>
#include <node-manager/RecoveryChecker.h>

#include <boost/filesystem.hpp>
#include <memory> // for std::auto_ptr

using namespace license_module;

namespace bfs = boost::filesystem;

namespace sf1r
{

CollectionManager::CollectionManager()
{
    RecoveryChecker::get()->setRestartCallback(
        boost::bind(&CollectionManager::startCollection, this, _1, _2, false),
        boost::bind(&CollectionManager::stopCollection, this, _1, _2));

}

CollectionManager::~CollectionManager()
{
    for(handler_const_iterator iter = handlerBegin(); iter != handlerEnd(); iter++)
    {
         delete iter->second;
    }

    for(CollectionMutexes::iterator iter = collectionMutexes_.begin();
        iter != collectionMutexes_.end(); ++iter)
    {
        delete iter->second;
    }
}

CollectionManager::MutexType* CollectionManager::getCollectionMutex(const std::string& collection)
{
    boost::mutex::scoped_lock lock(mapMutex_);

    MutexType*& mutex = collectionMutexes_[collection];
    if (mutex == NULL)
    {
        mutex = new MutexType;
    }

    return mutex;
}

bool CollectionManager::startCollection(const string& collectionName, const std::string& configFileName, bool fixBasePath)
{
    try{
    ScopedWriteLock lock(*getCollectionMutex(collectionName));

    if(findHandler(collectionName) != NULL)
        return false;

#ifdef COBRA_RESTRICT
    if (!LicenseCustManager::get()->hasCollection(collectionName))
    {
    	return false;
    }
    std::pair<uint32_t, uint32_t> date;
    uint32_t currentDate = license_tool::getCurrentDate();
    LicenseCustManager::get()->getDate(collectionName, date);
    if (date.first > currentDate || date.second < currentDate)
    {
    	return false;
    }
#endif

    LOG(INFO) << "Start Collection: " << collectionName;

    std::auto_ptr<CollectionHandler> collectionHandler(new CollectionHandler(collectionName));

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
    collectionHandler->setDocumentSchema(collectionMeta.getDocumentSchema());

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
    if (!productBundleConfig->mode_.empty())
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

        GetRecommendWorker* getRecommendWorker = static_cast<GetRecommendWorker*>(osgiLauncher_.getService(bundleName, "GetRecommendWorker"));
        collectionHandler->registerWorker(getRecommendWorker);

        UpdateRecommendWorker* updateRecommendWorker = static_cast<UpdateRecommendWorker*>(osgiLauncher_.getService(bundleName, "UpdateRecommendWorker"));
        collectionHandler->registerWorker(updateRecommendWorker);

        collectionHandler->setBundleSchema(recommendBundleConfig->recommendSchema_);
    }

    collectionHandlers_[collectionName] = collectionHandler.release();
    }catch (std::exception& e)
    {
        stopCollection(collectionName);
        throw;
    }
    RecoveryChecker::get()->updateCollection(*(SF1Config::get()));
    return true;
}

bool CollectionManager::stopCollection(const std::string& collectionName, bool clear)
{
    ScopedWriteLock lock(*getCollectionMutex(collectionName));

    LOG(INFO) << "Stop Collection: " << collectionName;

    JobScheduler::get()->removeTask(collectionName);

    CollectionPath cpath;
    handler_const_iterator iter = collectionHandlers_.find(collectionName);
    if(iter != collectionHandlers_.end())
    {
        CollectionHandler* handler = iter->second;
        IndexSearchService* is = handler->indexSearchService_;
        if(is==NULL) return false;
        const IndexBundleConfiguration* ibc = is->getBundleConfig();
        if(ibc==NULL) return false;
        cpath = ibc->collPath_;
        delete handler;
        collectionHandlers_.erase(collectionName);
    }
    else
    {
        return false;
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
    RecoveryChecker::get()->updateCollection(*(SF1Config::get()));
    if(clear)
    {
        namespace bfs = boost::filesystem;
        std::string collection_data = cpath.getCollectionDataPath();
        std::string query_data = cpath.getQueryDataPath();
        std::string scd_dir = cpath.getScdPath()+"/index";
        try
        {
            boost::filesystem::remove_all(collection_data);
            //boost::filesystem::remove_all(query_data);
            std::vector<std::string> scd_list;
            ScdParser::getScdList(scd_dir, scd_list);
            bfs::path bk_dir = bfs::path(scd_dir) / "backup";
            bfs::create_directory(bk_dir);

            LOG(INFO) << "moving " << scd_list.size() << " SCD files to directory " << bk_dir;
            for(uint32_t i=0;i<scd_list.size();i++)
            {
                bfs::path dest = bk_dir / bfs::path(scd_list[i]).filename();
                if(bfs::exists(dest))
                {
                    LOG(WARNING) << "dest in rename file " << scd_list[i] << " exists, delete it"<<std::endl;
                    bfs::remove_all(scd_list[i]);
                }
                else
                {
                    bfs::rename(scd_list[i], dest);
                }
            }
        }
        catch(std::exception& ex)
        {
            std::cerr<<"clear collection "<<collectionName<<" error: "<<ex.what()<<std::endl;
            return false;
        }
    }
    return true;
}

void CollectionManager::deleteCollection(const std::string& collectionName)
{

}

CollectionHandler* CollectionManager::findHandler(const std::string& key) const
{
    handler_const_iterator iter = collectionHandlers_.find(key);

    if (iter != collectionHandlers_.end())
        return iter->second;

    return NULL;
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
