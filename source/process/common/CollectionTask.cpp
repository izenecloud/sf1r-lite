#include "CollectionTask.h"

#include <controllers/CollectionHandler.h>
#include <common/CollectionManager.h>
#include <common/XmlConfigParser.h>
#include <common/JobScheduler.h>

#include <bundles/index/IndexTaskService.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;
namespace bfs3 = boost::filesystem3;

namespace sf1r
{

void RebuildTask::startTask()
{
    task_type task = boost::bind(&RebuildTask::doTask, this);
    JobScheduler::get()->addTask(task, rebuildCollectionName_);
}

void RebuildTask::doTask()
{
    if (isRunning_)
    {
        LOG(ERROR) << "RebuildTask is running!" ;
        return;
    }

    LOG(INFO) << "## start RebuildTask : " << collectionName_;
    isRunning_ = true;

    std::string collDir;
    std::string rebuildCollDir;
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
    if (!CollectionManager::get()->startCollection(rebuildCollectionName_, configFile, true))
    {
        LOG(ERROR) << "Collection for rebuilding already started: " << rebuildCollectionName_;
        isRunning_ = false;
        return;
    }
    CollectionManager::MutexType* recollMutex = CollectionManager::get()->getCollectionMutex(rebuildCollectionName_);
    CollectionManager::ScopedReadLock recollLock(*recollMutex);
    CollectionHandler* rebuildCollHandler = CollectionManager::get()->findHandler(rebuildCollectionName_);
    rebuildCollHandler->indexTaskService_->index(documentManager);
    CollectionPath& rebuildCollPath = rebuildCollHandler->indexTaskService_->getCollectionPath();
    rebuildCollDir = rebuildCollPath.getCollectionDataPath() + rebuildCollPath.getCurrCollectionDir();
    } // lock scope

    // replace collection data with rebuilded data
    LOG(INFO) << "## stopCollection: " << collectionName_;
    CollectionManager::get()->stopCollection(collectionName_);
    ///LOG(INFO) << "## stopCollection: " << rebuildCollectionName_;
    ///CollectionManager::get()->stopCollection(rebuildCollectionName_);

    bfs::remove_all(collDir+"-backup");
    bfs::rename(collDir, collDir+"-backup");
    try {
        bfs3::copy_directory(rebuildCollDir, collDir);
    }
    catch (const std::exception& e) {
        // rollbak
        bfs::rename(collDir+"-backup", collDir);
    }
    ///bfs::remove_all(rebuildCollPath.getBasePath());

    LOG(INFO) << "## startCollection: " << collectionName_;
    CollectionManager::get()->startCollection(collectionName_, configFile);

    LOG(INFO) << "## end RebuildTask : " << collectionName_;
    isRunning_ = false;
}

}
