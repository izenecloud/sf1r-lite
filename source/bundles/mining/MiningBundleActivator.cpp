#include "MiningBundleActivator.h"

#include <common/SFLogger.h>
#include <bundles/index/IndexSearchService.h>

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include <memory> // for auto_ptr

namespace bfs = boost::filesystem;

namespace sf1r
{

using namespace izenelib::osgi;
MiningBundleActivator::MiningBundleActivator()
    :tracker_(0)
    ,context_(0)
    ,searchService_(0)
    ,searchServiceReg_(0)
    ,taskService_(0)
    ,taskServiceReg_(0)
    ,config_(0)
{
}

MiningBundleActivator::~MiningBundleActivator()
{
}

void MiningBundleActivator::start( IBundleContext::ConstPtr context )
{
    context_ = context;

    boost::shared_ptr<BundleConfiguration> bundleConfigPtr = context->getBundleConfig();
    config_ = static_cast<MiningBundleConfiguration*>(bundleConfigPtr.get());
    tracker_ = new ServiceTracker( context, "IndexSearchService", this );
    tracker_->startTracking();
}

void MiningBundleActivator::stop( IBundleContext::ConstPtr context )
{
    if(tracker_)
    {
        tracker_->stopTracking();
        delete tracker_;
        tracker_ = 0;
    }
    if(searchServiceReg_)
    {
        searchServiceReg_->unregister();
        delete searchServiceReg_;
        delete searchService_;
        searchServiceReg_ = 0;
        searchService_ = 0;
    }

    if(taskServiceReg_)
    {
        taskServiceReg_->unregister();
        delete taskServiceReg_;
        delete taskService_;
        taskServiceReg_ = 0;
        taskService_ = 0;
    }
}

bool MiningBundleActivator::addingService( const ServiceReference& ref )
{
    if ( ref.getServiceName() == "IndexSearchService" )
    {
        Properties props = ref.getServiceProperties();
        if ( props.get( "collection" ) == config_->collectionName_)
        {
            IndexSearchService* service = reinterpret_cast<IndexSearchService*> ( const_cast<IService*>(ref.getService()) );
            cout << "[MiningBundleActivator#addingService] Calling IndexSearchService..." << endl;
            openDataDirectories_();
            miningManager_ = createMiningManager_(service);
            if (!miningManager_)
            {
                cerr << "error: failed in creating MiningManager" << endl;
                return false;
            }
            searchService_ = new MiningSearchService;
            searchService_->miningManager_ = miningManager_;
            
            
            taskService_ = new MiningTaskService;
            taskService_->miningManager_ = miningManager_;
            
            Properties props;
            props.put( "collection", config_->collectionName_);
            searchServiceReg_ = context_->registerService( "MiningSearchService", searchService_, props );
            taskServiceReg_ = context_->registerService( "MiningTaskService", taskService_, props );
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }

}

void MiningBundleActivator::removedService( const ServiceReference& ref )
{

}

std::string MiningBundleActivator::getCurrentCollectionDataPath_() const
{
    return config_->collPath_.getCollectionDataPath()+"/"+currentCollectionDataName_;
}

std::string MiningBundleActivator::getCollectionDataPath_() const
{
    return config_->collPath_.getCollectionDataPath();
}

std::string MiningBundleActivator::getQueryDataPath_() const
{
    return config_->collPath_.getQueryDataPath();
}


bool MiningBundleActivator::openDataDirectories_()
{
    std::vector<std::string>& directories = config_->collectionDataDirectories_;
    if( directories.size() == 0 )
    {
        std::cout<<"no data dir config"<<std::endl;
        return false;
    }
    directoryRotator_.setCapacity(directories.size());
    typedef std::vector<std::string>::const_iterator iterator;
    for (iterator it = directories.begin(); it != directories.end(); ++it)
    {
        bfs::path dataDir = bfs::path( getCollectionDataPath_() ) / *it;
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
	//std::cout << "Current Index Directory: " << indexPath_() << std::endl;
	return true;
    }
    return false;
}

boost::shared_ptr<MiningManager>
MiningBundleActivator::createMiningManager_(IndexSearchService* indexService) const
{
    std::string dir = getCurrentCollectionDataPath_()+"/mining";
    boost::filesystem::create_directories(dir);
    boost::shared_ptr<MiningManager> ret;

    ret.reset(
        new MiningManager(
            dir,
            getQueryDataPath_(),
            indexService->laManager_,
            indexService->documentManager_,
            indexService->indexManager_,
            indexService->searchManager_,
            indexService->idManager_,
            config_->collectionName_,
            config_->schema_,
            config_->mining_config_,
            config_->mining_schema_
        )
    );

    ret->system_resource_path_ = config_->system_resource_path_;

    bool succ = ret->open();
    if(!succ)
    {
      ret.reset();
    }
    return ret;
}

}

