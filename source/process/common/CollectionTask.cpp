#include "CollectionTask.h"

#include <controllers/CollectionHandler.h>
#include <common/CollectionManager.h>
#include <common/XmlConfigParser.h>
#include <common/Utilities.h>

#include <bundles/index/IndexTaskService.h>
#include <core/license-manager/LicenseCustManager.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/NodeManagerBase.h>
#include <node-manager/MasterManagerBase.h>
#include <node-manager/RecoveryChecker.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

const static std::string collectionCronJobName = "CollectionTaskScheduler-";
namespace sf1r
{

void CollectionTask::cronTask(int calltype)
{
    if( (isCronTask_ && cronExpression_.matches_now()) || calltype > 0)
    {
        if (calltype == 0 && NodeManagerBase::get()->isDistributed())
        {
            if (NodeManagerBase::get()->isPrimary())
            {
                MasterManagerBase::get()->pushWriteReq(collectionCronJobName + getTaskName(), "cron");
                LOG(INFO) << "push cron job to queue on primary : " << collectionCronJobName + getTaskName();
            }
            else
            {
                LOG(INFO) << "cron job on replica ignored. ";
            }
            return;
        }

        doTask();
    }
}

void RebuildTask::doTask()
{
    LOG(INFO) << "## start RebuildTask for " << collectionName_;

    std::string collDir;
    std::string rebuildCollDir;
    std::string rebuildCollBaseDir;
    std::string configFile = SF1Config::get()->getCollectionConfigFile(collectionName_);

    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;

    RebuildCronTaskReqLog reqlog;
    reqlog.cron_time = Utilities::createTimeStamp();
    if (!DistributeRequestHooker::get()->prepare(Req_CronJob, reqlog))
    {
        LOG(ERROR) << "!!!! prepare log failed while rebuild collection." << getTaskName() << std::endl;
        return;
    }

    {
    // check collection resource
    CollectionManager::MutexType* collMutex = CollectionManager::get()->getCollectionMutex(collectionName_);
    CollectionManager::ScopedReadLock collLock(*collMutex);
    CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(collectionName_);
    if (!collectionHandler || !collectionHandler->indexTaskService_)
    {
        LOG(ERROR) << "Not found collection: " << collectionName_;
        throw -1;
    }
    boost::shared_ptr<DocumentManager> documentManager = collectionHandler->indexTaskService_->getDocumentManager();
    CollectionPath& collPath = collectionHandler->indexTaskService_->getCollectionPath();
    collDir = collPath.getCollectionDataPath() + collPath.getCurrCollectionDir();

    CollectionHandler* rebuildCollHandler = CollectionManager::get()->findHandler(rebuildCollectionName_);
    if (rebuildCollHandler)
    {
        LOG(ERROR) << "Collection for rebuilding already started: " << rebuildCollectionName_;
        throw -1;
    }

    bfs::path basePath(collPath.getBasePath());
    if (basePath.filename().string() == ".")
        basePath = basePath.parent_path().parent_path();
    else
        basePath = basePath.parent_path();

    LOG(INFO) << "rebuild path : " << basePath/bfs::path(rebuildCollectionName_);
    if (bfs::exists(basePath/bfs::path(rebuildCollectionName_)))
    {
        LOG(INFO) << "the rebuild collection was not deleted properly last time, removing it!";
        bfs::remove_all(basePath/bfs::path(rebuildCollectionName_));
    }

    // start collection for rebuilding
    LOG(INFO) << "## startCollection for rebuilding: " << rebuildCollectionName_;
    if (!CollectionManager::get()->startCollection(rebuildCollectionName_, configFile, true))
    {
        LOG(ERROR) << "Collection for rebuilding already started: " << rebuildCollectionName_;
        throw -1;
    }
    CollectionManager::MutexType* recollMutex = CollectionManager::get()->getCollectionMutex(rebuildCollectionName_);
    CollectionManager::ScopedReadLock recollLock(*recollMutex);
    rebuildCollHandler = CollectionManager::get()->findHandler(rebuildCollectionName_);
    LOG(INFO) << "# # # #  start rebuilding";
    rebuildCollHandler->indexTaskService_->index(documentManager);
    CollectionPath& rebuildCollPath = rebuildCollHandler->indexTaskService_->getCollectionPath();
    rebuildCollDir = rebuildCollPath.getCollectionDataPath() + rebuildCollPath.getCurrCollectionDir();
    rebuildCollBaseDir = rebuildCollPath.getBasePath();
    } // lock scope

    DistributeRequestHooker::get()->setReplayingLog(true, reqlog);
    // replay log for rebuilded collection.
    RecoveryChecker::get()->replayLog(DistributeRequestHooker::get()->isRunningPrimary(),
        collectionName_, rebuildCollectionName_, reqlog.replayed_id_list);

    DistributeRequestHooker::get()->setReplayingLog(false, reqlog);
    // replace collection data with rebuilded data
    LOG(INFO) << "## stopCollection: " << collectionName_;
    CollectionManager::get()->stopCollection(collectionName_);
    LOG(INFO) << "## stopCollection: " << rebuildCollectionName_;
    CollectionManager::get()->stopCollection(rebuildCollectionName_);

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
        throw;
    }

    LOG(INFO) << "## re-startCollection: " << collectionName_;
    CollectionManager::get()->startCollection(collectionName_, configFile);

    LOG(INFO) << "## end RebuildTask for " << collectionName_;
    //isRunning_ = false;
    //
    DISTRIBUTE_WRITE_FINISH2(true, reqlog);
}

void ExpirationCheckTask::doTask()
{
    LOG(INFO) << "## start ExpirationCheckTask for " << collectionName_;
    //isRunning_ = true;

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
    //isRunning_ = false;
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
