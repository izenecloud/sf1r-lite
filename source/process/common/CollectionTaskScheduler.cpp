#include "CollectionTaskScheduler.h"
#include "CollectionTask.h"

#include <controllers/CollectionHandler.h>

#include <bundles/index/IndexTaskService.h>

#include <core/license-manager/LicenseCustManager.h>

namespace sf1r
{

const static std::string collectionCronJobName = "CollectionTaskScheduler";

CollectionTaskScheduler::CollectionTaskScheduler()
{
}

CollectionTaskScheduler::~CollectionTaskScheduler()
{
    for (TaskListType::const_iterator cit = taskList_.begin();
        cit != taskList_.end(); ++cit)
    {
        izenelib::util::Scheduler::removeJob(collectionCronJobName + (*cit)->getTaskName());
    }
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
            if (!izenelib::util::Scheduler::addJob(
                    collectionCronJobName + task->getTaskName(),
                    60*1000, // notify in every minute
                    0, // start with no delay
                    boost::bind(&RebuildTask::cronTask, task.get(), _1)))
            {
                LOG(ERROR) << "Failed to add cron job: " << collectionCronJobName + task->getTaskName();
            }
        }
        else
        {
            LOG(ERROR) << "Failed to set cron expression: " << cronStr;
            return false;
        }
    }

    return true;
}

bool CollectionTaskScheduler::scheduleLicenseTask(std::string collectionName)
{ // Check collection license expiration
	std::string cronStr = "0 3 * * *"; // start task at 3 am every day.

	std::pair<uint32_t, uint32_t> licenseDate;
	if (LicenseCustManager::get()->getDate(collectionName, licenseDate))
	{
		boost::shared_ptr<ExpirationCheckTask> task(new ExpirationCheckTask(collectionName, licenseDate));
		if (task && task->setCronExpression(cronStr))
		{
			task->setIsCronTask(true);
			taskList_.push_back(task);
            if (!izenelib::util::Scheduler::addJob(
                    collectionCronJobName + task->getTaskName(),
                    60*1000, // notify in every minute
                    0, // start with no delay
                    boost::bind(&ExpirationCheckTask::cronTask, task.get(), _1)))
            {
                LOG(ERROR) << "Failed to add cron job: " << collectionCronJobName + task->getTaskName();
                return false;
            }
		}
		else
		{
			LOG(ERROR) << "Failed to set cron expression: "<< cronStr<<" for ExpirationCheckTask";
			return false;
		}
	}
	else
	{
		LOG(ERROR) << "Failed to schedule license task as license date not found.";
		return false;
	}

	return true;
}

}
