#include "RecommendBundleActivator.h"
#include <recommend-manager/UserManager.h>
#include <recommend-manager/ItemManager.h>
#include <recommend-manager/VisitManager.h>
#include <recommend-manager/PurchaseManager.h>
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
    ,recommendManager_(NULL)
    ,userIdGenerator_(NULL)
    ,itemIdGenerator_(NULL)
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
    delete recommendManager_;
    delete userIdGenerator_;
    delete itemIdGenerator_;
    userManager_ = NULL;
    itemManager_ = NULL;
    visitManager_ = NULL;
    purchaseManager_ = NULL;
    recommendManager_ = NULL;
    userIdGenerator_ = NULL;
    itemIdGenerator_ = NULL;
}

bool RecommendBundleActivator::init_()
{
    openDataDirectories_();

    std::string dir = getCurrentCollectionDataPath_() + "/recommend/";
    boost::filesystem::create_directories(dir);

    std::string userPath = dir + "user.db";
    auto_ptr<UserManager> userManagerPtr(new UserManager(userPath));

    std::string itemPath = dir + "item.db";
    auto_ptr<ItemManager> itemManagerPtr(new ItemManager(itemPath));

    std::string visitPath = dir + "visit.db";
    auto_ptr<VisitManager> visitManagerPtr(new VisitManager(visitPath));

    std::string purchasePath = dir + "purchase.db";
    auto_ptr<PurchaseManager> purchaseManagerPtr(new PurchaseManager(purchasePath));

    auto_ptr<RecommendManager> recommendManagerPtr(new RecommendManager);

    std::string userIdPath = dir + "userid";
    auto_ptr<RecIdGenerator> userIdGeneratorPtr(new RecIdGenerator(userIdPath));

    std::string itemIdPath = dir + "itemid";
    auto_ptr<RecIdGenerator> itemIdGeneratorPtr(new RecIdGenerator(itemIdPath));

    userManager_ = userManagerPtr.release();
    itemManager_ = itemManagerPtr.release();
    visitManager_ = visitManagerPtr.release();
    purchaseManager_ = purchaseManagerPtr.release();
    recommendManager_ = recommendManagerPtr.release();
    userIdGenerator_ = userIdGeneratorPtr.release();
    itemIdGenerator_ = itemIdGeneratorPtr.release();

    taskService_ = new RecommendTaskService(config_, userManager_, itemManager_, userIdGenerator_, itemIdGenerator_);
    searchService_ = new RecommendSearchService(userManager_, itemManager_, userIdGenerator_, itemIdGenerator_);

    return true;
}

bool RecommendBundleActivator::openDataDirectories_()
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
	bfs::path p = newest->path();
	currentCollectionDataName_ = p.filename();
	return true;
    }

    return false;
}

std::string RecommendBundleActivator::getCurrentCollectionDataPath_() const
{
    return config_->collPath_.getCollectionDataPath()+"/"+currentCollectionDataName_;
}

} // namespace sf1r
