#ifndef INDEX_BUNDLE_ACTIVATOR_H
#define INDEX_BUNDLE_ACTIVATOR_H

#include "IndexBundleConfiguration.h"

#include <util/osgi/IBundleActivator.h>
#include <util/osgi/IBundleContext.h>
#include <util/osgi/IServiceRegistration.h>
#include <util/osgi/ServiceTracker.h>

namespace sf1r
{
using namespace izenelib::osgi;

class IndexBundleActivator : public IBundleActivator, public IServiceTrackerCustomizer
{
private:
    ServiceTracker* tracker_;
    IBundleContext* context_;
    IServiceRegistration* serviceReg_;
    IndexBundleConfiguration* config_;

public:
    IndexBundleActivator();
    virtual ~IndexBundleActivator();
    virtual void start( IBundleContext::ConstPtr context );
    virtual void stop( IBundleContext::ConstPtr context );
    virtual bool addingService( const ServiceReference& ref );
    virtual void removedService( const ServiceReference& ref );
};

}
#endif

