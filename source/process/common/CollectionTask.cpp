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

    // check collection resource
    CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(collectionName_);
    if (!collectionHandler || !collectionHandler->indexTaskService_)
    {
        LOG(ERROR) << "Not found collection: " << collectionName_;
        return;
    }

    boost::shared_ptr<DocumentManager> documentManager = collectionHandler->indexTaskService_->getDocumentManager();
    if (!documentManager)
    {
        LOG(ERROR) << "Failed to get documentManager for collection: " << collectionName_;
        return;
    }

    std::string configFile = SF1Config::get()->getCollectionConfigFile(collectionName_);
    CollectionPath& collPath = collectionHandler->indexTaskService_->getCollectionPath();

    // start collection for rebuilding
    if (CollectionManager::get()->findHandler(rebuildCollectionName_))
    {
        LOG(ERROR) << "Collection for rebuilding already started: " << rebuildCollectionName_;
        return;
    }

    CollectionHandler* rebuildCollHandler =
            CollectionManager::get()->startCollection(rebuildCollectionName_, configFile, true);
    if (!rebuildCollHandler || !rebuildCollHandler->indexTaskService_)
    {
        LOG(ERROR) << "Failed to start collection for rebuilding: " << rebuildCollectionName_;
        return;
    }

    rebuildCollHandler->indexTaskService_->index(documentManager);
    CollectionPath& rebuildCollPath = rebuildCollHandler->indexTaskService_->getCollectionPath();

    // replace collection data with rebuilded data
    LOG(INFO) << "## stopCollection: " << collectionName_;
    CollectionManager::get()->stopCollection(collectionName_);
    LOG(INFO) << "## stopCollection: " << rebuildCollectionName_;
    ///CollectionManager::get()->stopCollection(rebuildCollectionName_);

    std::string collDir = collPath.getCollectionDataPath() + collPath.getCurrCollectionDir();
    std::string rebuildCollDir = rebuildCollPath.getCollectionDataPath() + rebuildCollPath.getCurrCollectionDir();

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
