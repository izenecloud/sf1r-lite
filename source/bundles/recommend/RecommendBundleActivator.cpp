#include "RecommendBundleActivator.h"
#include <recommend-manager/UserManager.h>
#include <recommend-manager/ItemManager.h>
#include <recommend-manager/VisitManager.h>
#include <recommend-manager/PurchaseManager.h>
#include <recommend-manager/OrderManager.h>
#include <recommend-manager/RecommendManager.h>

#include <common/SFLogger.h>

#include <memory> // auto_ptr

#include <boost/filesystem.hpp>

namespace sf1r
{

RecommendBundleActivator::RecommendBundleActivator()
    :context_(NULL)
    ,taskService_(NULL)
    ,taskServiceReg_(NULL)
    ,searchService_(NULL)
    ,searchServiceReg_(NULL)
    ,userManager_(NULL)
    ,itemManager_(NULL)
    ,visitManager_(NULL)
    ,purchaseManager_(NULL)
    ,orderManager_(NULL)
    ,recommendManager_(NULL)
    ,userIdGenerator_(NULL)
    ,itemIdGenerator_(NULL)
    ,coVisitManager_(NULL)
    ,itemCFManager_(NULL)
{
}

RecommendBundleActivator::~RecommendBundleActivator()
{
}

void RecommendBundleActivator::start(IBundleContext::ConstPtr context)
{
    context_ = context;

    boost::shared_ptr<BundleConfiguration> bundleConfigPtr = context->getBundleConfig();
    config_ = static_cast<RecommendBundleConfiguration*>(bundleConfigPtr.get());

    init_();

    Properties props;
    props.put("collection", config_->collectionName_);
    taskServiceReg_ = context->registerService("RecommendTaskService", taskService_, props);
    searchServiceReg_ = context->registerService("RecommendSearchService", searchService_, props);
}

void RecommendBundleActivator::stop(IBundleContext::ConstPtr context)
{
    if (taskServiceReg_)
    {
        taskServiceReg_->unregister();
        delete taskServiceReg_;
        delete taskService_;
        taskServiceReg_ = NULL;
        taskService_ = NULL;
    }

    if (searchServiceReg_)
    {
        searchServiceReg_->unregister();
        delete searchServiceReg_;
        delete searchService_;
        searchServiceReg_ = NULL;
        searchService_ = NULL;
    }

    delete userManager_;
    delete itemManager_;
    delete visitManager_;
    delete purchaseManager_;
    delete orderManager_;
    delete recommendManager_;
    delete userIdGenerator_;
    delete itemIdGenerator_;
    delete coVisitManager_;
    delete itemCFManager_;
    userManager_ = NULL;
    itemManager_ = NULL;
    visitManager_ = NULL;
    purchaseManager_ = NULL;
    orderManager_ = NULL;
    recommendManager_ = NULL;
    userIdGenerator_ = NULL;
    itemIdGenerator_ = NULL;
    coVisitManager_ = NULL;
    itemCFManager_ = NULL;
}

bool RecommendBundleActivator::init_()
{
    // create SCD directories
    boost::filesystem::create_directories(config_->userSCDPath());
    boost::filesystem::create_directories(config_->itemSCDPath());
    boost::filesystem::create_directories(config_->orderSCDPath());

    // create data directory
    std::string dir;
    openDataDirectory_(dir);
    boost::filesystem::create_directories(dir);

    std::string userPath = dir + "/user.db";
    auto_ptr<UserManager> userManagerPtr(new UserManager(userPath));

    std::string itemPath = dir + "/item.db";
    std::string maxIdPath = dir + "/max_itemid.txt";
    auto_ptr<ItemManager> itemManagerPtr(new ItemManager(itemPath, maxIdPath));

    std::string covisitPath = dir + "/covisit";
    auto_ptr<CoVisitManager> coVisitManagerPtr(new CoVisitManager(covisitPath));

    std::string cfPath = dir + "/cf";
    boost::filesystem::create_directories(cfPath);
    auto_ptr<ItemCFManager> itemCFManagerPtr(new ItemCFManager(cfPath + "/covisit", 500*1024*1024,
                                                               cfPath + "/sim", 500*1024*1024,
                                                               cfPath + "/nb.sdb", 30,
                                                               cfPath + "/rec", 1000));

    std::string visitPath = dir + "/visit.db";
    auto_ptr<VisitManager> visitManagerPtr(new VisitManager(visitPath, coVisitManagerPtr.get()));

    std::string purchasePath = dir + "/purchase.db";
    auto_ptr<PurchaseManager> purchaseManagerPtr(new PurchaseManager(purchasePath, itemCFManagerPtr.get(), itemManagerPtr.get()));

    std::string orderPath = dir + "/order";
    auto_ptr<OrderManager> orderManagerPtr(new OrderManager(orderPath, itemManagerPtr.get()));

    auto_ptr<RecommendManager> recommendManagerPtr(new RecommendManager(itemManagerPtr.get(),
                                                                        visitManagerPtr.get(),
                                                                        coVisitManagerPtr.get(),
                                                                        itemCFManagerPtr.get(),
                                                                        orderManagerPtr.get()));

    std::string userIdPath = dir + "/userid";
    auto_ptr<RecIdGenerator> userIdGeneratorPtr(new RecIdGenerator(userIdPath));

    std::string itemIdPath = dir + "/itemid";
    auto_ptr<RecIdGenerator> itemIdGeneratorPtr(new RecIdGenerator(itemIdPath));

    userManager_ = userManagerPtr.release();
    itemManager_ = itemManagerPtr.release();
    visitManager_ = visitManagerPtr.release();
    purchaseManager_ = purchaseManagerPtr.release();
    orderManager_ = orderManagerPtr.release();
    recommendManager_ = recommendManagerPtr.release();

    userIdGenerator_ = userIdGeneratorPtr.release();
    itemIdGenerator_ = itemIdGeneratorPtr.release();

    coVisitManager_ = coVisitManagerPtr.release();
    itemCFManager_ = itemCFManagerPtr.release();

    taskService_ = new RecommendTaskService(config_, &directoryRotator_, userManager_, itemManager_, visitManager_, purchaseManager_, orderManager_, userIdGenerator_, itemIdGenerator_);
    searchService_ = new RecommendSearchService(userManager_, itemManager_, recommendManager_, userIdGenerator_, itemIdGenerator_);

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
    typedef std::vector<std::string>::const_iterator iterator;
    namespace bfs = boost::filesystem;
    bfs::path dataPath = config_->collPath_.getCollectionDataPath();
    for (iterator it = directories.begin(); it != directories.end(); ++it)
    {
        bfs::path dataDir = dataPath / *it;
        if (!directoryRotator_.appendDirectory(dataDir))
	{
	  std::string msg = dataDir.file_string() + " corrupted, delete it!";
	  sflog->error( SFL_SYS, msg.c_str() ); 
	  std::cout<<msg<<std::endl;
	  //clean the corrupt dir
	  boost::filesystem::remove_all( dataDir );
	  directoryRotator_.appendDirectory(dataDir);
	}
    }

    directoryRotator_.rotateToNewest();
    boost::shared_ptr<Directory> newest = directoryRotator_.currentDirectory();
    if (newest)
    {
	dataDir = newest->path().string();
	return true;
    }

    return false;
}

} // namespace sf1r
