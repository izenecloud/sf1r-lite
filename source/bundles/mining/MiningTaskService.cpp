#include "MiningTaskService.h"
#include <util/scheduler.h>
#include <util/driver/Request.h>
#include <node-manager/RequestLog.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/NodeManagerBase.h>
#include <node-manager/MasterManagerBase.h>

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
                                                            boost::bind(&MiningTaskService::cronJob_, this, _1));
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

void MiningTaskService::DoContinue()
{
    miningManager_->DoContinue();
}

void MiningTaskService::cronJob_(int calltype)
{
    if (cronExpression_.matches_now() || calltype > 0)
    {
	if(calltype == 0)
	{
		if (NodeManagerBase::get()->isPrimary() && !DistributeRequestHooker::get()->isHooked())
		{
			MasterManagerBase::get()->pushWriteReq(cronJobName_, "cron");
			LOG(INFO) << "push cron job to queue on primary : " << cronJobName_;
		}
		else
			LOG(INFO) << "cron job ignored on replica: " << cronJobName_;
		return;
	}
        if (!DistributeRequestHooker::get()->isValid())
        {
            LOG(INFO) << "cron job ignored : " << cronJobName_;
            return;
        }
        CronJobReqLog reqlog;
        if (!DistributeRequestHooker::get()->prepare(Req_CronJob, reqlog))
        {
            LOG(ERROR) << "!!!! prepare log failed while running cron job. : " << cronJobName_ << std::endl;
            return;
        }

        DoMiningCollection();
	DistributeRequestHooker::get()->processLocalFinished(true);
    }
}

}

