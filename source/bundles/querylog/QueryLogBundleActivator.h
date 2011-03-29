#ifndef INDEX_BUNDLE_ACTIVATOR_H
#define INDEX_BUNDLE_ACTIVATOR_H

#include "QueryLogBundleConfiguration.h"

#include <util/osgi/IBundleActivator.h>
#include <util/osgi/IBundleContext.h>
#include <util/osgi/IServiceRegistration.h>
#include <util/osgi/ServiceTracker.h>

namespace sf1r
{
using namespace izenelib::osgi;

class QueryLogBundleActivator : public IBundleActivator, public IServiceTrackerCustomizer
{
private:
    ServiceTracker* tracker_;
    IBundleContext* context_;
    IServiceRegistration* serviceReg_;
    QueryLogBundleConfiguration* config_;
public:
    QueryLogBundleActivator();
    virtual ~QueryLogBundleActivator();
    virtual void start( IBundleContext::ConstPtr context );
    virtual void stop( IBundleContext::ConstPtr context );
    virtual bool addingService( const ServiceReference& ref );
    virtual void removedService( const ServiceReference& ref );
};

}
#endif

