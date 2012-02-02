#include "MiningTaskService.h"
#include <util/scheduler.h>

namespace sf1r
{
MiningTaskService::MiningTaskService(MiningBundleConfiguration* bundleConfig)
    :bundleConfig_(bundleConfig)
    ,cronJobName_("MiningTaskService-" + bundleConfig->collectionName_)
{
    if(!bundleConfig_->mining_config_.dcmin_param.cron.empty())
    {
        if(cronExpression_.setExpression(bundleConfig_->mining_config_.dcmin_param.cron))
        {
            bool result = izenelib::util::Scheduler::addJob(cronJobName_,
                                                            60*1000, // each minute
                                                            0, // start from now
                                                            boost::bind(&MiningTaskService::cronJob_, this));
            if (!result)
            {
                LOG(ERROR) << "failed in izenelib::util::Scheduler::addJob(), cron job name: " << cronJobName_;
            }
        }
    }
}

MiningTaskService::~MiningTaskService()
{
    izenelib::util::Scheduler::removeJob(cronJobName_);
}

void MiningTaskService::DoMiningCollection()
{
    miningManager_->DoMiningCollection();
}

void MiningTaskService::cronJob_()
{
    if (cronExpression_.matches_now())
    {
        DoMiningCollection();
    }
}

}

