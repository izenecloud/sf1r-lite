#include "RecommendBundleActivator.h"
#include "RecommendBundleConfiguration.h"

#include <recommend-manager/item/LocalItemFactory.h>
#include <recommend-manager/item/SearchMasterItemFactory.h>
#include <recommend-manager/item/RemoteItemFactory.h>
#include <recommend-manager/item/ItemManager.h>
#include <recommend-manager/item/ItemIdGenerator.h>
#include <recommend-manager/storage/RecommendStorageFactory.h>
#include <recommend-manager/storage/UserManager.h>
#include <recommend-manager/storage/VisitManager.h>
#include <recommend-manager/storage/PurchaseManager.h>
#include <recommend-manager/storage/CartManager.h>
#include <recommend-manager/storage/RateManager.h>
#include <recommend-manager/storage/EventManager.h>
#include <recommend-manager/storage/OrderManager.h>
#include <recommend-manager/recommender/RecommenderFactory.h>

#include <aggregator-manager/GetRecommendMaster.h>
#include <aggregator-manager/GetRecommendWorker.h>
#include <aggregator-manager/UpdateRecommendMaster.h>
#include <aggregator-manager/UpdateRecommendWorker.h>
#include <node-manager/RecommendNodeManager.h>
#include <node-manager/sharding/RecommendShardStrategy.h>
#include <node-manager/sharding/RecommendMatrixSharder.h>

#include <glog/logging.h>

namespace bfs = boost::filesystem;

namespace sf1r
{

RecommendBundleActivator::RecommendBundleActivator()
    :context_(NULL)
    ,getRecommendBase_(NULL)
    ,updateRecommendBase_(NULL)
{
}

RecommendBundleActivator::~RecommendBundleActivator()
{
}

void RecommendBundleActivator::start(IBundleContext::ConstPtr context)
{
    context_ = context;
    config_ = static_pointer_cast<RecommendBundleConfiguration>(context_->getBundleConfig());

    if (config_->recommendNodeConfig_.isServiceNode() &&
        config_->searchNodeConfig_.isServiceNode())
    {
        indexSearchTracker_.reset(new ServiceTracker(context, "IndexSearchService", this));
        indexSearchTracker_->startTracking();
    }
    else
    {
        init_(NULL);
    }
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

    if (getWorkerReg_)
    {
        getWorkerReg_->unregister();
        getWorkerReg_.reset();
    }

    if (updateWorkerReg_)
    {
        updateWorkerReg_->unregister();
        updateWorkerReg_.reset();
    }

    itemManager_.reset();
    itemIdGenerator_.reset();

    userManager_.reset();
    visitManager_.reset();
    purchaseManager_.reset();
    cartManager_.reset();
    orderManager_.reset();
    queryPurchaseCounter_.reset();
    eventManager_.reset();
    rateManager_.reset();

    recommenderFactory_.reset();

    getRecommendBase_ = NULL;
    getRecommendWorker_.reset();
    getRecommendMaster_.reset();

    updateRecommendBase_ = NULL;
    updateRecommendWorker_.reset();
    updateRecommendMaster_.reset();

    shardStrategy_.reset();
    matrixSharder_.reset();

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
                LOG(ERROR) << "failed in RecommendBundleActivator::init_(), collection: " << config_->collectionName_;
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
    if (! createDataDir_())
        return false;

    const DistributedNodeConfig& nodeConfig = config_->recommendNodeConfig_;

    if (nodeConfig.isMasterNode_ || nodeConfig.isWorkerNode_)
        createSharder_();

    if (nodeConfig.isSingleNode_ || nodeConfig.isWorkerNode_)
        createWorker_();

    if (nodeConfig.isMasterNode_)
        createMaster_();

    if (nodeConfig.isServiceNode())
    {
        createSCDDir_();
        createStorage_();
        createItem_(indexSearchService);
        createOrder_();
        createClickCounter_();
        createRecommender_();
        createService_();
    }

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
    eventManager_.reset(storageFactory.createEventManager());
}

void RecommendBundleActivator::createItem_(IndexSearchService* indexSearchService)
{
    boost::scoped_ptr<ItemFactory> factory;
    const DistributedNodeConfig& searchNode = config_->searchNodeConfig_;

    if (searchNode.isSingleNode_)
    {
        factory.reset(new LocalItemFactory(*indexSearchService));
    }
    else if (searchNode.isMasterNode_)
    {
        factory.reset(new SearchMasterItemFactory(config_->collectionName_, *indexSearchService));
    }
    else
    {
        factory.reset(new RemoteItemFactory(config_->collectionName_));
    }

    itemIdGenerator_.reset(factory->createItemIdGenerator());
    itemManager_.reset(factory->createItemManager());
}

void RecommendBundleActivator::createSharder_()
{
    const RecommendNodeManager* nodeManager = RecommendNodeManager::get();
    const Sf1rNode& sf1rNode = nodeManager->getCurrentSf1rNode();

    shardid_t shardNum = sf1rNode.master_.totalShardNum_;
    shardStrategy_.reset(new RecommendShardMod(shardNum));

    if (config_->recommendNodeConfig_.isWorkerNode_)
    {
        shardid_t workerShardId = sf1rNode.worker_.shardId_;
        matrixSharder_.reset(new RecommendMatrixSharder(workerShardId, shardStrategy_.get()));
    }
}

void RecommendBundleActivator::createWorker_()
{
    bfs::path miningDir = dataDir_ / "mining";
    bfs::create_directory(miningDir);

    bfs::path cfPath = miningDir / "cf";
    bfs::create_directory(cfPath);

    idmlib::recommender::ItemCFParam itemCFParam(cfPath.string());
    const std::size_t matrixSize = config_->purchaseCacheSize_ >> 1;
    itemCFParam.coVisitCacheSize = matrixSize;
    itemCFParam.simMatrixCacheSize = matrixSize;

    itemCFManager_.reset(new ItemCFManager(itemCFParam, matrixSharder_.get()));

    coVisitManager_.reset(new CoVisitManager((miningDir / "covisit").string(),
                                             config_->visitCacheSize_, matrixSharder_.get()));

    getRecommendWorker_.reset(new GetRecommendWorker(*itemCFManager_, *coVisitManager_));
    getRecommendBase_ = getRecommendWorker_.get();

    updateRecommendWorker_.reset(new UpdateRecommendWorker(*itemCFManager_, *coVisitManager_));
    updateRecommendBase_ = updateRecommendWorker_.get();

    Properties props;
    props.put("collection", config_->collectionName_);
    getWorkerReg_.reset(context_->registerService("GetRecommendWorker", getRecommendWorker_.get(), props));
    updateWorkerReg_.reset(context_->registerService("UpdateRecommendWorker", updateRecommendWorker_.get(), props));
}

void RecommendBundleActivator::createMaster_()
{
    getRecommendMaster_.reset(
        new GetRecommendMaster(config_->collectionName_, shardStrategy_.get(), getRecommendWorker_.get()));
    getRecommendBase_ = getRecommendMaster_.get();

    updateRecommendMaster_.reset(
        new UpdateRecommendMaster(config_->collectionName_, updateRecommendWorker_.get()));
    updateRecommendBase_ = updateRecommendMaster_.get();
}

void RecommendBundleActivator::createOrder_()
{
    bfs::path orderDir = dataDir_ / "order";
    bfs::create_directory(orderDir);

    orderManager_.reset(new OrderManager(orderDir.string(),
                                         config_->indexCacheSize_));

    orderManager_->setMinThreshold(config_->itemSetMinFreq_);
}

void RecommendBundleActivator::createClickCounter_()
{
    bfs::path counterDir = dataDir_ / "click_counter";
    bfs::path counterPath = counterDir / "query_purchase.db";
    bfs::create_directory(counterDir);

    queryPurchaseCounter_.reset(new QueryClickCounter(counterPath.string()));
}

void RecommendBundleActivator::createRecommender_()
{
    recommenderFactory_.reset(new RecommenderFactory(*itemIdGenerator_,
                                                     *visitManager_, *purchaseManager_,
                                                     *cartManager_, *orderManager_,
                                                     *eventManager_, *rateManager_,
                                                     *queryPurchaseCounter_,
                                                     *getRecommendBase_));
}

void RecommendBundleActivator::createService_()
{
    taskService_.reset(new RecommendTaskService(*config_, directoryRotator_, *userManager_, *itemManager_,
                                                *visitManager_, *purchaseManager_, *cartManager_, *orderManager_,
                                                *eventManager_, *rateManager_, *itemIdGenerator_, *queryPurchaseCounter_,
                                                *updateRecommendBase_, updateRecommendWorker_.get()));

    searchService_.reset(new RecommendSearchService(*config_, *userManager_, *itemManager_,
                                                    *recommenderFactory_, *itemIdGenerator_));

    Properties props;
    props.put("collection", config_->collectionName_);
    taskServiceReg_.reset(context_->registerService("RecommendTaskService", taskService_.get(), props));
    searchServiceReg_.reset(context_->registerService("RecommendSearchService", searchService_.get(), props));
}

} // namespace sf1r
