#include "ProductBundleActivator.h"
#include "ProductSearchService.h"
#include "ProductTaskService.h"
#include "ProductIndexHooker.h"

#include <bundles/index/IndexTaskService.h>
#include <bundles/index/IndexSearchService.h>

#include <common/SFLogger.h>
#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>
#include <product-manager/product_manager.h>
#include <product-manager/collection_product_data_source.h>
#include <product-manager/scd_operation_processor.h>
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
    ,refIndexTaskService_(0)
    ,config_(0)
    ,data_source_(0)
    ,op_processor_(0)
{
}

ProductBundleActivator::~ProductBundleActivator()
{
    if(data_source_!=0)
    {
        delete data_source_;
    }
    if(op_processor_!=0)
    {
        delete op_processor_;
    }
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
            IndexSearchService* service = reinterpret_cast<IndexSearchService*> ( const_cast<IService*>(ref.getService()) );
            std::cout << "[ProductBundleActivator#addingService] Calling IndexSearchService..." << std::endl;
            productManager_ = createProductManager_(service);
            
            searchService_ = new ProductSearchService(config_);
            searchService_->productManager_ = productManager_;
            
            if(refIndexTaskService_)
            {
                addIndexHook_(refIndexTaskService_);
            }
            
            taskService_ = new ProductTaskService(config_);
            return true;
        }
        else
        {
            return false;
        }
    }
    else if( ref.getServiceName() == "IndexTaskService" )
    {
        refIndexTaskService_ = reinterpret_cast<IndexTaskService*> ( const_cast<IService*>(ref.getService()) );
        if(productManager_)
        {
            addIndexHook_(refIndexTaskService_);
        }
    }
    else
    {
        return false;
    }
    return true;
}

boost::shared_ptr<ProductManager> 
ProductBundleActivator::createProductManager_(IndexSearchService* indexService)
{
    std::string dir = getCurrentCollectionDataPath_()+"/product";
    boost::filesystem::create_directories(dir);
    PMConfig config = PMConfig::GetDefaultPMConfig();
    data_source_ = new CollectionProductDataSource(indexService->workerService_->documentManager_, indexService->workerService_->indexManager_, config);
    op_processor_ = new ScdOperationProcessor(dir);
    boost::shared_ptr<ProductManager> product_manager(new ProductManager(data_source_, op_processor_, config));
    return product_manager;
    
}

void ProductBundleActivator::addIndexHook_(IndexTaskService* indexService) const
{
    indexService->hooker_.reset(new ProductIndexHooker(productManager_));
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



}

