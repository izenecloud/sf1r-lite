#include "MiningBundleActivator.h"

#include <common/SFLogger.h>
#include <bundles/index/IndexSearchService.h>

#include <mining-manager/query-correction-submanager/QueryCorrectionSubmanager.h>
#include <aggregator-manager/SearchWorker.h>

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include <memory> // for auto_ptr

namespace bfs = boost::filesystem;

namespace sf1r
{

using namespace izenelib::osgi;

MiningBundleActivator::MiningBundleActivator()
    : tracker_(0)
    , context_(0)
    , searchService_(0)
    , searchServiceReg_(0)
    , taskService_(0)
    , taskServiceReg_(0)
    , config_(0)
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
    tracker_ = new ServiceTracker( context, "IndexSearchService", this );
    tracker_->startTracking();
}

void MiningBundleActivator::stop( IBundleContext::ConstPtr context )
{
    if(tracker_)
    {
        tracker_->stopTracking();
        delete tracker_;
        tracker_ = 0;
    }
    if(searchServiceReg_)
    {
        searchServiceReg_->unregister();
        delete searchServiceReg_;
        delete searchService_;
        searchServiceReg_ = 0;
        searchService_ = 0;
    }

    if(taskServiceReg_)
    {
        taskServiceReg_->unregister();
        delete taskServiceReg_;
        delete taskService_;
        taskServiceReg_ = 0;
        taskService_ = 0;
    }
}

bool MiningBundleActivator::addingService( const ServiceReference& ref )
{
    if ( ref.getServiceName() == "IndexSearchService" )
    {
        Properties props = ref.getServiceProperties();
        if ( props.get( "collection" ) == config_->collectionName_)
        {
            IndexSearchService* service = reinterpret_cast<IndexSearchService*> ( const_cast<IService*>(ref.getService()) );
            cout << "[MiningBundleActivator#addingService] Calling IndexSearchService..." << endl;
            currentCollectionDataName_ = service->bundleConfig_->collPath_.getCurrCollectionDir();
            miningManager_ = createMiningManager_(service);
            if (!miningManager_)
            {
                cerr << "error: failed in creating MiningManager" << endl;
                return false;
            }
            searchService_ = new MiningSearchService;
            searchService_->bundleConfig_ = config_;
            searchService_->miningManager_ = miningManager_;
            searchService_->searchAggregator_ = service->searchAggregator_;
            searchService_->searchWorker_ = service->searchWorker_;

            taskService_ = new MiningTaskService(config_);
            taskService_->miningManager_ = miningManager_;

            Properties props;
            props.put( "collection", config_->collectionName_);
            searchServiceReg_ = context_->registerService( "MiningSearchService", searchService_, props );
            taskServiceReg_ = context_->registerService( "MiningTaskService", taskService_, props );
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }

}

void MiningBundleActivator::removedService( const ServiceReference& ref )
{

}

std::string MiningBundleActivator::getCurrentCollectionDataPath_() const
{
    return config_->collPath_.getCollectionDataPath()+"/"+currentCollectionDataName_;
}

std::string MiningBundleActivator::getCollectionDataPath_() const
{
    return config_->collPath_.getCollectionDataPath();
}

std::string MiningBundleActivator::getQueryDataPath_() const
{
    return config_->collPath_.getQueryDataPath();
}


boost::shared_ptr<MiningManager>
MiningBundleActivator::createMiningManager_(IndexSearchService* indexService) const
{
    std::string dir = getCurrentCollectionDataPath_()+"/mining";
    boost::filesystem::create_directories(dir);
    boost::shared_ptr<MiningManager> ret;

    ret.reset(
            new MiningManager(
                dir,
                getQueryDataPath_(),
                indexService->searchWorker_->documentManager_,
                indexService->searchWorker_->indexManager_,
                indexService->searchWorker_->searchManager_,
                indexService->searchWorker_->idManager_,
                config_->collectionName_,
                config_->documentSchema_,
                config_->mining_config_,
                config_->mining_schema_
                )
            );

    static bool FirstRun = true;
    if (FirstRun)
    {
        FirstRun = false;

        MiningManager::system_resource_path_ = config_->system_resource_path_;
        MiningManager::system_working_path_ = config_->system_working_path_;
    }

    bool succ = ret->open();
    if(!succ)
    {
        ret.reset();
    }
    return ret;
}

}
