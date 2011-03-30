#ifndef MINING_BUNDLE_ACTIVATOR_H
#define MINING_BUNDLE_ACTIVATOR_H

#include "MiningBundleConfiguration.h"

#include <util/osgi/IBundleActivator.h>
#include <util/osgi/IBundleContext.h>
#include <util/osgi/IServiceRegistration.h>
#include <util/osgi/ServiceTracker.h>

namespace sf1r
{
using namespace izenelib::osgi;

class MiningBundleActivator : public IBundleActivator, public IServiceTrackerCustomizer
{
private:
    ServiceTracker* tracker_;
    IBundleContext* context_;
    IServiceRegistration* serviceReg_;
    MiningBundleConfiguration* config_;

public:
    MiningBundleActivator();
    virtual ~MiningBundleActivator();
    virtual void start( IBundleContext::ConstPtr context );
    virtual void stop( IBundleContext::ConstPtr context );
    virtual bool addingService( const ServiceReference& ref );
    virtual void removedService( const ServiceReference& ref );
};

}
#endif

