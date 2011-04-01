#include "OSGILauncher.h"
#include <bundles/index/IndexTaskService.h>

#include <util/osgi/BundleInfo.h>
#include <util/osgi/ServiceInfo.h>

namespace sf1r
{
void OSGILauncher::start(boost::shared_ptr<BundleConfiguration> bundleConfig)
{
    logger_.log( Logger::LOG_DEBUG, "[Launcher#start] Called." );
    this->objectCreator_.setSearchConfiguration( true,
            bundleConfig->getLibraryPath(), bundleConfig->getLibraryName() );

    logger_.log( Logger::LOG_DEBUG, "[Launcher#start] Reading configuration: Library path: %1, class name: %2",
                bundleConfig->getLibraryPath(), bundleConfig->getClassName() );

    IBundleActivator* bundleActivator = NULL;

    try
    {
        bundleActivator = this->objectCreator_.createObject( bundleConfig->getClassName() );
    }
    catch ( ObjectCreationException &exc )
    {
        std::string msg( exc.what() );
        logger_.log( Logger::LOG_ERROR, "[Launcher#start] Error during loading bundle activator, exc: %1", msg );
        try{
            bundleActivator = this->objectCreator_.createObject( bundleConfig->getBundleName() );
        }catch ( ObjectCreationException &exc )
        {
            std::string msg( exc.what() );
            logger_.log( Logger::LOG_ERROR, "[Launcher#start] Error during loading bundle activator, exc: %1", msg );
        }
    }

    IBundleContext* bundleCtxt = this->createBundleContext( bundleConfig->getBundleName() );
    bundleCtxt->bindConfiguration(bundleConfig);
    BundleInfoBase* bundleInfo = new BundleInfo( bundleConfig->getBundleName(), false, bundleActivator, bundleCtxt );
    this->registry_->addBundleInfo( (*bundleInfo) );
    logger_.log( Logger::LOG_DEBUG, "[Launcher#start] Start bundle." );

    bundleActivator->start( bundleCtxt );
}

IService* OSGILauncher::getService(const std::string& bundleName, const std::string& serviceName)
{
    std::vector<ServiceInfoPtr> services = this->getRegistry().getBundleInfo(bundleName)->getRegisteredServices();

    for(size_t i = 0; i < services.size(); ++i)
    {
        if(services[i]->getServiceName() == serviceName)
        {
            IService* service = services[i]->getService();
            return service;
	}
    }
    return NULL;
}

}

