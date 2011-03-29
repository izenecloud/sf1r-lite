#include "IndexBundleActivator.h"

#include <boost/shared_ptr.hpp>

namespace sf1r
{

using namespace izenelib::osgi;
IndexBundleActivator::IndexBundleActivator()
    :tracker_(0)
    ,context_(0)
    ,serviceReg_(0)
    ,config_(0)
{
}

IndexBundleActivator::~IndexBundleActivator()
{
}

void IndexBundleActivator::start( IBundleContext::ConstPtr context )
{
    context_ = context;

    boost::shared_ptr<BundleConfiguration> bundleConfigPtr = context->getBundleConfig();
    config_ = static_cast<IndexBundleConfiguration*>(bundleConfigPtr.get());

}

void IndexBundleActivator::stop( IBundleContext::ConstPtr context )
{
}

bool IndexBundleActivator::addingService( const ServiceReference& ref )
{
    return true;
}

void IndexBundleActivator::removedService( const ServiceReference& ref )
{

}


}

