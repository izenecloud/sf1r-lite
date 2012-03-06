#include "CollectionTaskScheduler.h"
#include "CollectionTaskType.h"

#include <controllers/CollectionHandler.h>

#include <bundles/index/IndexTaskService.h>

namespace sf1r
{

const static std::string collectionCronJobName = "CollectionTaskScheduler";

CollectionTaskScheduler::CollectionTaskScheduler()
{
    if (!izenelib::util::Scheduler::addJob(
            collectionCronJobName,
            5*1000, // 5*60*1000
            20*1000, // 5*60*1000
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

    // check rebuild task
    if (collectionHandler->indexTaskService_->isAutoRebuild())
    {
        std::string collectionName = collectionHandler->indexTaskService_->bundleConfig_->collectionName_;
        std::string cronStr = collectionHandler->indexTaskService_->bundleConfig_->cronIndexer_;
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
        boost::shared_ptr<CollectionTaskType>& task = *it;
        if (task->isCronTask())
        {
            if (task->getCronExpression().matches_now())
            {
                task->doTask();
            }
        }
    }
}

void CollectionTaskScheduler::onTimer_()
{
}

}
