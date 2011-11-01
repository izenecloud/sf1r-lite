#ifndef MINING_BUNDLE_ACTIVATOR_H
#define MINING_BUNDLE_ACTIVATOR_H

#include "MiningBundleConfiguration.h"
#include "MiningSearchService.h"
#include "MiningTaskService.h"
#include <directory-manager/DirectoryRotator.h>

#include <util/osgi/IBundleActivator.h>
#include <util/osgi/IBundleContext.h>
#include <util/osgi/IServiceRegistration.h>
#include <util/osgi/ServiceTracker.h>

namespace sf1r
{
using namespace izenelib::osgi;

class IndexSearchService;
class MiningBundleActivator : public IBundleActivator, public IServiceTrackerCustomizer
{
private:
    ServiceTracker* tracker_;
    IBundleContext* context_;
    MiningSearchService* searchService_;
    IServiceRegistration* searchServiceReg_;
    MiningTaskService* taskService_;
    IServiceRegistration* taskServiceReg_;

    MiningBundleConfiguration* config_;
    std::string currentCollectionDataName_;

    DirectoryRotator directoryRotator_;

    boost::shared_ptr<MiningManager> miningManager_;

    bool openDataDirectories_();

    boost::shared_ptr<MiningManager> createMiningManager_(IndexSearchService* indexService) const;

    std::string getCollectionDataPath_() const;

    std::string getCurrentCollectionDataPath_() const;

    std::string getQueryDataPath_() const;

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
