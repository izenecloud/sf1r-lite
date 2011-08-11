#include "RecommendBundleActivator.h"
#include <recommend-manager/UserManager.h>
#include <recommend-manager/ItemManager.h>
#include <recommend-manager/VisitManager.h>
#include <recommend-manager/PurchaseManager.h>
#include <recommend-manager/CartManager.h>
#include <recommend-manager/EventManager.h>
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
    ,cartManager_(NULL)
    ,eventManager_(NULL)
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
    delete cartManager_;
    delete eventManager_;
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
    cartManager_ = NULL;
    eventManager_ = NULL;
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
    boost::filesystem::path dataDir(dir);
    boost::filesystem::create_directories(dataDir);

    boost::filesystem::path userDir = dataDir / "user";
    boost::filesystem::create_directory(userDir);
    auto_ptr<UserManager> userManagerPtr(new UserManager((userDir / "user.db").string()));

    boost::filesystem::path itemDir = dataDir / "item";
    boost::filesystem::create_directory(itemDir);
    auto_ptr<ItemManager> itemManagerPtr(new ItemManager((itemDir / "item.db").string(),
                                                         (itemDir / "max_itemid.txt").string()));

    boost::filesystem::path miningDir = dataDir / "mining";
    boost::filesystem::create_directory(miningDir);

    boost::filesystem::path cfPath = miningDir / "cf";
    boost::filesystem::create_directory(cfPath);
    auto_ptr<ItemCFManager> itemCFManagerPtr(new ItemCFManager((cfPath / "covisit").string(), 500*1024*1024,
                                                               (cfPath / "sim").string(), 500*1024*1024,
                                                               (cfPath / "nb.sdb").string(), 30,
                                                               (cfPath / "rec").string(), 1000));

    auto_ptr<CoVisitManager> coVisitManagerPtr(new CoVisitManager((miningDir / "covisit").string()));

    boost::filesystem::path eventDir = dataDir / "event";
    boost::filesystem::create_directory(eventDir);

    auto_ptr<VisitManager> visitManagerPtr(new VisitManager((eventDir / "visit.db").string(),
                                                            (eventDir / "visit_session.db").string(),
                                                            coVisitManagerPtr.get()));

    auto_ptr<PurchaseManager> purchaseManagerPtr(new PurchaseManager((eventDir / "purchase.db").string(),
                                                                     itemCFManagerPtr.get(),
                                                                     itemManagerPtr.get()));

    auto_ptr<CartManager> cartManagerPtr(new CartManager((eventDir / "cart.db").string()));

    auto_ptr<EventManager> eventManagerPtr(new EventManager((eventDir / "event.db").string()));

    boost::filesystem::path orderDir = dataDir / "order";
    boost::filesystem::create_directory(orderDir);
    auto_ptr<OrderManager> orderManagerPtr(new OrderManager(orderDir.string(), itemManagerPtr.get()));

    auto_ptr<RecommendManager> recommendManagerPtr(new RecommendManager(config_->recommendSchema_,
                                                                        itemManagerPtr.get(),
                                                                        visitManagerPtr.get(),
                                                                        purchaseManagerPtr.get(),
                                                                        cartManagerPtr.get(),
                                                                        eventManagerPtr.get(),
                                                                        coVisitManagerPtr.get(),
                                                                        itemCFManagerPtr.get(),
                                                                        orderManagerPtr.get()));

    boost::filesystem::path idDir = dataDir / "id";
    boost::filesystem::create_directory(idDir);

    auto_ptr<RecIdGenerator> userIdGeneratorPtr(new RecIdGenerator((idDir / "userid").string()));
    auto_ptr<RecIdGenerator> itemIdGeneratorPtr(new RecIdGenerator((idDir / "itemid").string()));

    userManager_ = userManagerPtr.release();
    itemManager_ = itemManagerPtr.release();
    visitManager_ = visitManagerPtr.release();
    purchaseManager_ = purchaseManagerPtr.release();
    cartManager_ = cartManagerPtr.release();
    eventManager_ = eventManagerPtr.release();
    orderManager_ = orderManagerPtr.release();
    recommendManager_ = recommendManagerPtr.release();

    userIdGenerator_ = userIdGeneratorPtr.release();
    itemIdGenerator_ = itemIdGeneratorPtr.release();

    coVisitManager_ = coVisitManagerPtr.release();
    itemCFManager_ = itemCFManagerPtr.release();

    taskService_ = new RecommendTaskService(config_, &directoryRotator_, userManager_, itemManager_, visitManager_, purchaseManager_, cartManager_, eventManager_, orderManager_, userIdGenerator_, itemIdGenerator_);
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
    boost::filesystem::path dataPath = config_->collPath_.getCollectionDataPath();
    for (iterator it = directories.begin(); it != directories.end(); ++it)
    {
        boost::filesystem::path dir = dataPath / *it;
        if (!directoryRotator_.appendDirectory(dir))
	{
	  std::string msg = dir.file_string() + " corrupted, delete it!";
	  sflog->error( SFL_SYS, msg.c_str() ); 
	  std::cout<<msg<<std::endl;
	  //clean the corrupt dir
	  boost::filesystem::remove_all(dir);
	  directoryRotator_.appendDirectory(dir);
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
