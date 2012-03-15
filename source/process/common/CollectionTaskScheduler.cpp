#include "CollectionTaskScheduler.h"
#include "CollectionTask.h"

#include <controllers/CollectionHandler.h>

#include <bundles/index/IndexTaskService.h>

namespace sf1r
{

const static std::string collectionCronJobName = "CollectionTaskScheduler";

CollectionTaskScheduler::CollectionTaskScheduler()
{
    if (!izenelib::util::Scheduler::addJob(
            collectionCronJobName,
            60*1000, // notify in every minute
            0, // start with no delay
            boost::bind(&CollectionTaskScheduler::cronTask_, this)))
    {
        LOG(ERROR) << "Failed to add cron job: " << collectionCronJobName;
    }
}

CollectionTaskScheduler::~CollectionTaskScheduler()
{
    izenelib::util::Scheduler::removeJob(collectionCronJobName);
}

bool CollectionTaskScheduler::schedule(const CollectionHandler* collectionHandler)
{
    if (!collectionHandler)
    {
        LOG(ERROR) << "Failed to schedule collection task with NULL handler.";
        return false;
    }

    IndexTaskService* indexTaskService = collectionHandler->indexTaskService_;
    if (!indexTaskService)
    {
        LOG(ERROR) << "Failed to schedule collection task as IndexTaskService not found.";
        return false;
    }

    // check rebuild task
    if (indexTaskService->isAutoRebuild())
    {
        std::string collectionName = indexTaskService->bundleConfig_->collectionName_;
        std::string cronStr = indexTaskService->bundleConfig_->cronIndexer_;
        //std::string cronStr = "0  3   *   *   5"; // start task at 3 am on Fridays.

        boost::shared_ptr<RebuildTask> task(new RebuildTask(collectionName));
        if (task && task->setCronExpression(cronStr))
        {
            task->setIsCronTask(true);
            taskList_.push_back(task);
        }
        else
        {
            LOG(ERROR) << "Failed to set cron expression: " << cronStr;
            return false;
        }
    }

    return true;
}

void CollectionTaskScheduler::cronTask_()
{
    TaskListType::iterator it;
    for (it = taskList_.begin(); it != taskList_.end(); it++)
    {
        boost::shared_ptr<CollectionTask>& task = *it;
        if (task->isCronTask())
        {
            // cron expression will always match during the specified minute,
            // so the check interval should be 1 minute
            if (task->getCronExpression().matches_now())
            {
                task->startTask();
            }
        }
    }
}

void CollectionTaskScheduler::onTimer_()
{
}

}
