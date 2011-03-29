#include "MiningBundleActivator.h"

#include <boost/shared_ptr.hpp>

namespace sf1r
{

using namespace izenelib::osgi;
MiningBundleActivator::MiningBundleActivator()
    :tracker_(0)
    ,context_(0)
    ,serviceReg_(0)
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

}

void MiningBundleActivator::stop( IBundleContext::ConstPtr context )
{
}

bool MiningBundleActivator::addingService( const ServiceReference& ref )
{
}

void MiningBundleActivator::removedService( const ServiceReference& ref )
{

}


}

