/*
 * MiningQueryLogHandler.cpp
 *
 *  Created on: 2009-11-25
 *      Author: jinglei
 */

#include "MiningQueryLogHandler.h"

#include <util/scheduler.h>

using namespace sf1r;


MiningQueryLogHandler::MiningQueryLogHandler()
    : waitSec_(3600)
    , days_(7)
    , cron_started_(false)
{
}

MiningQueryLogHandler::~MiningQueryLogHandler()
{
    izenelib::util::Scheduler::removeJob("MiningQueryLogHandler");
}

void MiningQueryLogHandler::SetParam(uint32_t wait_sec, uint32_t days)
{
    waitSec_ = wait_sec;
    days_ = days;
}

void MiningQueryLogHandler::addCollection(const std::string& name, const boost::shared_ptr<RecommendManager>& recommendManager)
{
    boost::mutex::scoped_lock lock(mtx_);
    recommendManagerList_.insert(std::make_pair(name, recommendManager));
    collectionSet_.insert(name);
}

void MiningQueryLogHandler::runEvents()
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
        mlit->second->RebuildForAll();
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
    boost::function<void (void)> task = boost::bind(&MiningQueryLogHandler::cronJob_, this);
    izenelib::util::Scheduler::addJob("MiningQueryLogHandler", 60 * 1000, 0, task);
    cron_started_ = true;
    return true;
}

void MiningQueryLogHandler::cronJob_()
{
    if (cron_expression_.matches_now())
    {
        runEvents();
    }
}
