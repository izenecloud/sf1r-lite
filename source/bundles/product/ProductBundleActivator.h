#ifndef PRODUCT_BUNDLE_ACTIVATOR_H
#define PRODUCT_BUNDLE_ACTIVATOR_H

#include "ProductSearchService.h"
#include "ProductTaskService.h"

#include <common/type_defs.h>
#include <directory-manager/DirectoryRotator.h>
#include <util/osgi/IBundleActivator.h>
#include <util/osgi/IBundleContext.h>
#include <util/osgi/IServiceRegistration.h>
#include <util/osgi/ServiceTracker.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
using namespace izenelib::osgi;

class ProductManager;
class ProductDataSource;
class OperationProcessor;
class ProductScdReceiver;
class IndexTaskService;
class IndexSearchService;

class ProductBundleActivator : public IBundleActivator, public IServiceTrackerCustomizer
{
private:
    ServiceTracker* searchTracker_;
    ServiceTracker* taskTracker_;

    IBundleContext* context_;
    ProductSearchService* searchService_;
    IServiceRegistration* searchServiceReg_;
    ProductTaskService* taskService_;
    IServiceRegistration* taskServiceReg_;
    IndexTaskService* refIndexTaskService_;
    
    ProductBundleConfiguration* config_;
    std::string currentCollectionDataName_;
    DirectoryRotator directoryRotator_;
    ProductDataSource* data_source_;
    OperationProcessor* op_processor_;
    boost::shared_ptr<ProductManager> productManager_;
    ProductScdReceiver* scd_receiver_;
    bool openDataDirectories_();
    std::string getCollectionDataPath_() const;
    
    std::string getCurrentCollectionDataPath_() const;
    
    std::string getQueryDataPath_() const;
    
    boost::shared_ptr<ProductManager> createProductManager_(IndexSearchService* indexService);
    void addIndexHook_(IndexTaskService* indexService) const;

public:
    ProductBundleActivator();
    virtual ~ProductBundleActivator();
    virtual void start( IBundleContext::ConstPtr context );
    virtual void stop( IBundleContext::ConstPtr context );
    virtual bool addingService( const ServiceReference& ref );
    virtual void removedService( const ServiceReference& ref );
};

}
#endif

