#include "QueryLogBundleActivator.h"

#include <util/osgi/ObjectCreator.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{

using namespace izenelib::osgi;
QueryLogBundleActivator::QueryLogBundleActivator()
    :tracker_(0)
    ,context_(0)
    ,serviceReg_(0)
    ,config_(0)
{
}

QueryLogBundleActivator::~QueryLogBundleActivator()
{
}

void QueryLogBundleActivator::start( IBundleContext::ConstPtr context )
{
    context_ = context;

    boost::shared_ptr<BundleConfiguration> bundleConfigPtr = context->getBundleConfig();
    config_ = static_cast<QueryLogBundleConfiguration*>(bundleConfigPtr.get());

}

void QueryLogBundleActivator::stop( IBundleContext::ConstPtr context )
{
}

bool QueryLogBundleActivator::addingService( const ServiceReference& ref )
{
}

void QueryLogBundleActivator::removedService( const ServiceReference& ref )
{

}

REGISTER_BUNDLE_ACTIVATOR_CLASS( "QueryLogBundle", QueryLogBundleActivator )

}

