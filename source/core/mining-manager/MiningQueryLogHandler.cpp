/*
 * MiningQueryLogHandler.cpp
 *
 *  Created on: 2009-11-25
 *      Author: jinglei
 */

#include "MiningQueryLogHandler.h"
#include "query-recommend-submanager/RecommendManager.h"
#include <node-manager/RequestLog.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/NodeManagerBase.h>
#include <node-manager/MasterManagerBase.h>
#include <common/Utilities.h>


#include <util/scheduler.h>

using namespace sf1r;

static const std::string cronJobName = "MiningQueryLogHandler";

MiningQueryLogHandler::MiningQueryLogHandler()
    : waitSec_(3600)
    , days_(7)
    , cron_started_(false)
{
}

MiningQueryLogHandler::~MiningQueryLogHandler()
{
    izenelib::util::Scheduler::removeJob(cronJobName);
}

void MiningQueryLogHandler::SetParam(uint32_t wait_sec, uint32_t days)
{
    waitSec_ = wait_sec;
    days_ = days;
}

void MiningQueryLogHandler::addCollection(
        const std::string& name,
        const boost::shared_ptr<RecommendManager>& recommendManager)
{
    boost::mutex::scoped_lock lock(mtx_);
    recommendManagerList_.insert(std::make_pair(name, recommendManager));
    collectionSet_.insert(name);
}

void MiningQueryLogHandler::deleteCollection(const std::string& name)
{
    boost::mutex::scoped_lock lock(mtx_);
    if(recommendManagerList_.find(name) != recommendManagerList_.end())
        recommendManagerList_.erase(name);
    if(collectionSet_.find(name) != collectionSet_.end())
        collectionSet_.erase(name);
}


void MiningQueryLogHandler::runEvents(int64_t cron_time)
{
    boost::unique_lock<boost::mutex> lock(mtx_, boost::try_to_lock);
    if (!lock.owns_lock())
    {
        std::cout << "MiningQueryLogHandler locks" << std::endl;
        return;
    }
    std::cout << "[Start to update query information]" << std::endl;

    //collection specific feature: query recommend, query reminder, query correction
    for (map_it_type mlit = recommendManagerList_.begin();
            mlit != recommendManagerList_.end(); ++mlit)
    {
        mlit->second->RebuildForAll(cron_time);
    }

    std::cout << "[Updated query information]" << std::endl;
}

bool MiningQueryLogHandler::cronStart(const std::string& cron_job)
{
    if (cron_started_)
    {
        return true;
    }
    std::cout << "MiningQueryLogHandler cron starting : " << cron_job << std::endl;
    if (!cron_expression_.setExpression(cron_job))
    {
        return false;
    }
    boost::function<void (int)> task = boost::bind(&MiningQueryLogHandler::cronJob_, this, _1);
    izenelib::util::Scheduler::addJob(cronJobName, 60 * 1000, 0, task);
    cron_started_ = true;
    return true;
}

void MiningQueryLogHandler::cronJob_(int calltype)
{
    if (cron_expression_.matches_now() || calltype > 0)
    {
        if (calltype == 0 && NodeManagerBase::get()->isDistributed())
        {
            if (NodeManagerBase::get()->isPrimary())
            {
                MasterManagerBase::get()->pushWriteReq(cronJobName, "cron");
                LOG(INFO) << "push cron job to queue on primary : " << cronJobName;
            }
            else
            {
                LOG(INFO) << "cron job on replica ignored. ";
            }
            return;
        }
        if (!DistributeRequestHooker::get()->isValid())
        {
            LOG(INFO) << "cron job ignored : " << cronJobName;
            return;
        }

        CronJobReqLog reqlog;
        reqlog.cron_time = sf1r::Utilities::createTimeStamp(boost::posix_time::microsec_clock::local_time());

        if (!DistributeRequestHooker::get()->prepare(Req_CronJob, reqlog))
        {
            LOG(ERROR) << "!!!! prepare log failed while running cron job. : " << cronJobName << std::endl;
            return;
        }
        runEvents(reqlog.cron_time);
        DistributeRequestHooker::get()->processLocalFinished(true);
    }
}
