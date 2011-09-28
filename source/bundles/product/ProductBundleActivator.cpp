#include "ProductBundleActivator.h"
#include "ProductSearchService.h"
#include "ProductTaskService.h"

#include <bundles/index/IndexTaskService.h>
#include <bundles/index/IndexSearchService.h>

#include <common/SFLogger.h>
#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>
#include <util/singleton.h>

#include <boost/filesystem.hpp>

#include <memory> // for auto_ptr

namespace bfs = boost::filesystem;
using namespace izenelib::util;

namespace sf1r
{

using namespace izenelib::osgi;
ProductBundleActivator::ProductBundleActivator()
    :searchTracker_(0)
    ,taskTracker_(0)
    ,context_(0)
    ,searchService_(0)
    ,searchServiceReg_(0)
    ,taskService_(0)
    ,taskServiceReg_(0)
    ,config_(0)
{
}

ProductBundleActivator::~ProductBundleActivator()
{
}

void ProductBundleActivator::start( IBundleContext::ConstPtr context )
{
    context_ = context;

    boost::shared_ptr<BundleConfiguration> bundleConfigPtr = context->getBundleConfig();
    config_ = static_cast<ProductBundleConfiguration*>(bundleConfigPtr.get());

    Properties props;
    props.put( "collection", config_->collectionName_);
    searchServiceReg_ = context->registerService( "ProductSearchService", searchService_, props );
    taskServiceReg_ = context->registerService( "ProductTaskService", taskService_, props );
    searchTracker_ = new ServiceTracker( context, "IndexSearchService", this );
    searchTracker_->startTracking();
    taskTracker_ = new ServiceTracker( context, "IndexTaskService", this );
    taskTracker_->startTracking();
}

void ProductBundleActivator::stop( IBundleContext::ConstPtr context )
{
    if(searchTracker_)
    {
        searchTracker_->stopTracking();
        delete searchTracker_;
        searchTracker_ = 0;
    }
    if(taskTracker_)
    {
        taskTracker_->stopTracking();
        delete taskTracker_;
        taskTracker_ = 0;
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

bool ProductBundleActivator::addingService( const ServiceReference& ref )
{
    if ( ref.getServiceName() == "IndexSearchService" )
    {
        Properties props = ref.getServiceProperties();
        if ( props.get( "collection" ) == config_->collectionName_)
        {
            searchService_ = new ProductSearchService(config_);
            //IndexSearchService* service = reinterpret_cast<IndexSearchService*> ( const_cast<IService*>(ref.getService()) );
            cout << "[ProductBundleActivator#addingService] Calling IndexSearchService..." << endl;
            ///TODO
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ( ref.getServiceName() == "IndexTaskService" )	
    {
        Properties props = ref.getServiceProperties();
        if ( props.get( "collection" ) == config_->collectionName_)
        {
            taskService_ = new ProductTaskService(config_);
            IndexTaskService* service = reinterpret_cast<IndexTaskService*> ( const_cast<IService*>(ref.getService()) );
            cout << "[ProductBundleActivator#addingService] Calling IndexTaskService..." << endl;
            taskService_->indexTaskService_= service;
            ///TODO
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

void ProductBundleActivator::removedService( const ServiceReference& ref )
{

}


std::string ProductBundleActivator::getCurrentCollectionDataPath_() const
{
    return config_->collPath_.getCollectionDataPath()+"/"+currentCollectionDataName_;
}

std::string ProductBundleActivator::getCollectionDataPath_() const
{
    return config_->collPath_.getCollectionDataPath();
}

std::string ProductBundleActivator::getQueryDataPath_() const
{
    return config_->collPath_.getQueryDataPath();
}

bool ProductBundleActivator::openDataDirectories_()
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
	  DLOG(ERROR) <<msg <<endl;		  
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


}

