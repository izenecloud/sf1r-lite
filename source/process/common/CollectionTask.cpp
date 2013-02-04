#include "CollectionTask.h"

#include <controllers/CollectionHandler.h>
#include <common/CollectionManager.h>
#include <common/XmlConfigParser.h>

#include <bundles/index/IndexTaskService.h>
#include <core/license-manager/LicenseCustManager.h>
#include <node-manager/DistributeRequestHooker.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

namespace sf1r
{

void RebuildTask::startTask()
{
    if (isRunning_)
    {
        LOG(ERROR) << "RebuildTask is running!" ;
        return;
    }

    task_type task = boost::bind(&RebuildTask::doTask, this);
    asyncJodScheduler_.addTask(task);
}

void RebuildTask::doTask()
{
    LOG(INFO) << "## start RebuildTask for " << collectionName_;
    isRunning_ = true;

    std::string collDir;
    std::string rebuildCollDir;
    std::string rebuildCollBaseDir;
    std::string configFile = SF1Config::get()->getCollectionConfigFile(collectionName_);

    {
    // check collection resource
    CollectionManager::MutexType* collMutex = CollectionManager::get()->getCollectionMutex(collectionName_);
    CollectionManager::ScopedReadLock collLock(*collMutex);
    CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(collectionName_);
    if (!collectionHandler || !collectionHandler->indexTaskService_)
    {
        LOG(ERROR) << "Not found collection: " << collectionName_;
        isRunning_ = false;
        return;
    }
    boost::shared_ptr<DocumentManager> documentManager = collectionHandler->indexTaskService_->getDocumentManager();
    CollectionPath& collPath = collectionHandler->indexTaskService_->getCollectionPath();
    collDir = collPath.getCollectionDataPath() + collPath.getCurrCollectionDir();

    // start collection for rebuilding
    LOG(INFO) << "## startCollection for rebuilding: " << rebuildCollectionName_;
    if (!CollectionManager::get()->startCollection(rebuildCollectionName_, configFile, true))
    {
        LOG(ERROR) << "Collection for rebuilding already started: " << rebuildCollectionName_;
        isRunning_ = false;
        return;
    }
    CollectionManager::MutexType* recollMutex = CollectionManager::get()->getCollectionMutex(rebuildCollectionName_);
    CollectionManager::ScopedReadLock recollLock(*recollMutex);
    CollectionHandler* rebuildCollHandler = CollectionManager::get()->findHandler(rebuildCollectionName_);
    LOG(INFO) << "# # # #  start rebuilding";
    if (DistributeRequestHooker::get()->isHooked())
    {
        // this is chain request because index is another kind of write request
        if(!DistributeRequestHooker::get()->setChainStatus(DistributeRequestHooker::ChainMiddle))
        {
            LOG(ERROR) << "Collection for rebuilding set chain status failed: " << rebuildCollectionName_;
            
            isRunning_ = false;
            throw std::runtime_error("faile to rebuild");
            return;
        }
    }
    rebuildCollHandler->indexTaskService_->index(documentManager);
    CollectionPath& rebuildCollPath = rebuildCollHandler->indexTaskService_->getCollectionPath();
    rebuildCollDir = rebuildCollPath.getCollectionDataPath() + rebuildCollPath.getCurrCollectionDir();
    rebuildCollBaseDir = rebuildCollPath.getBasePath();
    } // lock scope

    // replace collection data with rebuilded data
    LOG(INFO) << "## stopCollection: " << collectionName_;
    CollectionManager::get()->stopCollection(collectionName_);
    LOG(INFO) << "## stopCollection: " << rebuildCollectionName_;
    CollectionManager::get()->stopCollection(rebuildCollectionName_);

    if (DistributeRequestHooker::get()->isHooked())
    {
        // this is chain request because index is another kind of write request
        if(!DistributeRequestHooker::get()->setChainStatus(DistributeRequestHooker::ChainEnd))
        {
            LOG(ERROR) << "Collection for rebuilding set chain status failed: " << rebuildCollectionName_;
            
            isRunning_ = false;
            throw std::runtime_error("faile to rebuild");
            return;
        }
    }

    LOG(INFO) << "## update collection data for " << collectionName_;
    try
    {
        bfs::remove_all(collDir+"-backup");
        bfs::rename(collDir, collDir+"-backup");
        try {
            bfs::rename(rebuildCollDir, collDir);
        }
        catch (const std::exception& e) {
            LOG(ERROR) << "failed to move data, rollback";
            bfs::rename(collDir+"-backup", collDir);
        }

        bfs::remove(collDir+"/scdlogs");
        bfs::copy_file(collDir+"-backup/scdlogs", collDir+"/scdlogs");
        bfs::remove_all(rebuildCollBaseDir);
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << e.what();
    }

    LOG(INFO) << "## re-startCollection: " << collectionName_;
    CollectionManager::get()->startCollection(collectionName_, configFile);

    LOG(INFO) << "## end RebuildTask for " << collectionName_;
    isRunning_ = false;
}

void ExpirationCheckTask::startTask()
{
    if (isRunning_)
    {
        LOG(ERROR) << "ExpirationCheckTask is running!" ;
        return;
    }

    task_type task = boost::bind(&ExpirationCheckTask::doTask, this);
    asyncJodScheduler_.addTask(task);
}

void ExpirationCheckTask::doTask()
{
    LOG(INFO) << "## start ExpirationCheckTask for " << collectionName_;
    isRunning_ = true;

    std::string configFile = SF1Config::get()->getCollectionConfigFile(collectionName_);
    uint32_t currentDate = license_module::license_tool::getCurrentDate();
    if (currentDate >= startDate_ && endDate_ >= currentDate)
    {
    	// Check collection handler
        if (!checkCollectionHandler(collectionName_))
        {
        	CollectionManager::get()->startCollection(collectionName_, configFile);
        }
    }
    // Check if the time is expired
    if (endDate_ < currentDate)
    {
    	CollectionManager::get()->stopCollection(collectionName_);
    	LOG(INFO) << "## deleteCollectionInfo: " << collectionName_;
    	LicenseCustManager::get()->deleteCollection(collectionName_);
    	setIsCronTask(false);
    }
    LOG(INFO) << "## end ExpirationCheckTask for " << collectionName_;
    isRunning_ = false;
}

bool ExpirationCheckTask::checkCollectionHandler(const std::string& collectionName) const
{
	CollectionManager::MutexType* collMutex = CollectionManager::get()->getCollectionMutex(collectionName);
	CollectionManager::ScopedReadLock collLock(*collMutex);

	CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(collectionName);
	if (collectionHandler != NULL)
		return true;
	return false;
}

}
