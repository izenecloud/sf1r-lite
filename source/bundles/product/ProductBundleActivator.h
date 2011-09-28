#ifndef PRODUCT_BUNDLE_ACTIVATOR_H
#define PRODUCT_BUNDLE_ACTIVATOR_H

#include "ProductSearchService.h"
#include "ProductTaskService.h"

#include <common/type_defs.h>

#include <util/osgi/IBundleActivator.h>
#include <util/osgi/IBundleContext.h>
#include <util/osgi/IServiceRegistration.h>
#include <util/osgi/ServiceTracker.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
using namespace izenelib::osgi;

class IndexManager;
class DocumentManager;

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

    ProductBundleConfiguration* config_;
    std::string currentCollectionDataName_;

    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<DocumentManager> documentManager_;
    boost::shared_ptr<IndexManager> indexManager_;
    DirectoryRotator directoryRotator_;

    bool openDataDirectories_();

    std::string getCollectionDataPath_() const;
    
    std::string getCurrentCollectionDataPath_() const;
    
    std::string getQueryDataPath_() const;

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

