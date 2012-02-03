#include "RecommendBundleActivator.h"
#include "RecommendBundleConfiguration.h"
#include <recommend-manager/ItemManager.h>
#include <recommend-manager/storage/RecommendStorageFactory.h>
#include <recommend-manager/storage/UserManager.h>
#include <recommend-manager/storage/VisitManager.h>
#include <recommend-manager/storage/PurchaseManager.h>
#include <recommend-manager/storage/CartManager.h>
#include <recommend-manager/storage/RateManager.h>
#include <recommend-manager/OrderManager.h>
#include <recommend-manager/EventManager.h>
#include <recommend-manager/RecommenderFactory.h>
#include <recommend-manager/ItemIdGenerator.h>
#include <bundles/index/IndexSearchService.h>

#include <aggregator-manager/SearchWorker.h>
#include <ir/id_manager/IDManager.h>
#include <common/SFLogger.h>

#include <memory> // auto_ptr
#include <glog/logging.h>

namespace bfs = boost::filesystem;

using izenelib::ir::idmanager::IDManager;

namespace sf1r
{

RecommendBundleActivator::RecommendBundleActivator()
    :context_(NULL)
{
}

RecommendBundleActivator::~RecommendBundleActivator()
{
}

void RecommendBundleActivator::start(IBundleContext::ConstPtr context)
{
    context_ = context;
    config_ = static_pointer_cast<RecommendBundleConfiguration>(context_->getBundleConfig());

    indexSearchTracker_.reset(new ServiceTracker(context, "IndexSearchService", this));
    indexSearchTracker_->startTracking();
}

void RecommendBundleActivator::stop(IBundleContext::ConstPtr context)
{
    if(indexSearchTracker_)
    {
        indexSearchTracker_->stopTracking();
        indexSearchTracker_.reset();
    }

    if (taskServiceReg_)
    {
        taskServiceReg_->unregister();
        taskServiceReg_.reset();
        taskService_.reset();
    }

    if (searchServiceReg_)
    {
        searchServiceReg_->unregister();
        searchServiceReg_.reset();
        searchService_.reset();
    }

    userManager_.reset();
    itemManager_.reset();
    visitManager_.reset();
    purchaseManager_.reset();
    cartManager_.reset();
    orderManager_.reset();
    eventManager_.reset();
    rateManager_.reset();
    recommenderFactory_.reset();
    itemIdGenerator_.reset();
    coVisitManager_.reset();
    itemCFManager_.reset();
}

bool RecommendBundleActivator::addingService(const ServiceReference& ref)
{
    if (ref.getServiceName() == "IndexSearchService")
    {
        Properties props = ref.getServiceProperties();
        if (props.get("collection") == config_->collectionName_)
        {
            IndexSearchService* service = reinterpret_cast<IndexSearchService*>(const_cast<IService*>(ref.getService()));
            if (! init_(service))
            {
                LOG(ERROR) << "failed in RecommendBundleActivator::init_(), collection:  " << config_->collectionName_;
                return false;
            }

            return true;
        }
    }

    return false;
}

void RecommendBundleActivator::removedService(const ServiceReference& ref)
{
}

bool RecommendBundleActivator::init_(IndexSearchService* indexSearchService)
{
    createSCDDir_();

    if (! createDataDir_())
        return false;

    createStorage_();
    createItem_(indexSearchService);
    createMining_();
    createEvent_();
    createOrder_();
    createRecommender_();
    createService_();

    return true;
}

bool RecommendBundleActivator::createDataDir_()
{
    std::string dir;
    if (! openDataDirectory_(dir))
    {
        LOG(ERROR) << "failed in RecommendBundleActivator::openDataDirectory_(), collection: "
                   << config_->collectionName_;
        return false;
    }

    dataDir_ = dir;
    bfs::create_directories(dataDir_);
    return true;
}

bool RecommendBundleActivator::openDataDirectory_(std::string& dataDir)
{
    std::vector<std::string>& directories = config_->collectionDataDirectories_;
    if (directories.size() == 0)
    {
        std::cout<<"no data dir config"<<std::endl;
        return false;
    }

    directoryRotator_.setCapacity(directories.size());
    std::vector<bfs::path> dirtyDirectories;
    typedef std::vector<std::string>::const_iterator iterator;
    bfs::path dataPath = config_->collPath_.getCollectionDataPath();
    for (iterator it = directories.begin(); it != directories.end(); ++it)
    {
        bfs::path dir = dataPath / *it;
        if (!directoryRotator_.appendDirectory(dir))
        {
            std::string msg = dir.string() + " corrupted, delete it!";
            LOG(ERROR) <<msg <<endl;
            //clean the corrupt dir
            bfs::remove_all(dir);
            dirtyDirectories.push_back(dir);
        }
    }

    directoryRotator_.rotateToNewest();
    boost::shared_ptr<Directory> newest = directoryRotator_.currentDirectory();
    if (newest)
    {
        dataDir = newest->path().string();
        std::vector<bfs::path>::iterator it = dirtyDirectories.begin();
        for( ; it != dirtyDirectories.end(); ++it)
            directoryRotator_.appendDirectory(*it);
        return true;
    }
    else
    {
        std::vector<bfs::path>::iterator it = dirtyDirectories.begin();
        for( ; it != dirtyDirectories.end(); ++it)
            directoryRotator_.appendDirectory(*it);
		
        directoryRotator_.rotateToNewest();
        boost::shared_ptr<Directory> dir = directoryRotator_.currentDirectory();
        if(dir)
        {
            dataDir = dir->path().string();
            return true;
        }
    }

    return false;
}

void RecommendBundleActivator::createSCDDir_()
{
    bfs::create_directories(config_->userSCDPath());
    bfs::create_directories(config_->orderSCDPath());
}

void RecommendBundleActivator::createStorage_()
{
    RecommendStorageFactory storageFactory(config_->cassandraConfig_,
                                           config_->collectionName_,
                                           dataDir_.string());

    userManager_.reset(storageFactory.createUserManager());
    purchaseManager_.reset(storageFactory.createPurchaseManager());
    visitManager_.reset(storageFactory.createVisitManager());
    cartManager_.reset(storageFactory.createCartManager());
    rateManager_.reset(storageFactory.createRateManager());
}

void RecommendBundleActivator::createItem_(IndexSearchService* indexSearchService)
{
    DocumentManager* docManager = indexSearchService->searchWorker_->documentManager_.get();
    itemManager_.reset(new ItemManager(docManager));

    IDManager* idManager = indexSearchService->searchWorker_->idManager_.get();
    itemIdGenerator_.reset(new ItemIdGenerator(idManager));
}

void RecommendBundleActivator::createMining_()
{
    bfs::path miningDir = dataDir_ / "mining";
    bfs::create_directory(miningDir);

    bfs::path cfPath = miningDir / "cf";
    bfs::create_directory(cfPath);
    const std::size_t matrixSize = config_->purchaseCacheSize_ >> 1;
    itemCFManager_.reset(new ItemCFManager((cfPath / "covisit").string(), matrixSize,
                                           (cfPath / "sim").string(), matrixSize,
                                           (cfPath / "nb.sdb").string(), 30));

    coVisitManager_.reset(new CoVisitManager((miningDir / "covisit").string(),
                                             config_->visitCacheSize_));
}

void RecommendBundleActivator::createEvent_()
{
    bfs::path eventDir = dataDir_ / "event";
    bfs::create_directory(eventDir);

    eventManager_.reset(new EventManager((eventDir / "event.db").string()));
}

void RecommendBundleActivator::createOrder_()
{
    bfs::path orderDir = dataDir_ / "order";
    bfs::create_directory(orderDir);

    orderManager_.reset(new OrderManager(orderDir.string(),
                                         config_->indexCacheSize_));

    orderManager_->setMinThreshold(config_->itemSetMinFreq_);

}

void RecommendBundleActivator::createRecommender_()
{
    recommenderFactory_.reset(new RecommenderFactory(*itemManager_,
                                                     *visitManager_, *purchaseManager_,
                                                     *cartManager_, *orderManager_,
                                                     *eventManager_, *rateManager_,
                                                     *coVisitManager_, *itemCFManager_));
}

void RecommendBundleActivator::createService_()
{
    taskService_.reset(new RecommendTaskService(*config_, directoryRotator_, *userManager_, *itemManager_,
                                                *visitManager_, *purchaseManager_, *cartManager_, *orderManager_,
                                                *eventManager_, *rateManager_, *itemIdGenerator_,
                                                *coVisitManager_, *itemCFManager_));

    searchService_.reset(new RecommendSearchService(*userManager_, *itemManager_,
                                                    *recommenderFactory_, *itemIdGenerator_));

    Properties props;
    props.put("collection", config_->collectionName_);
    taskServiceReg_.reset(context_->registerService("RecommendTaskService", taskService_.get(), props));
    searchServiceReg_.reset(context_->registerService("RecommendSearchService", searchService_.get(), props));
}

} // namespace sf1r
