/*
 * QueryDataBase.cpp
 *
 *  Created on: 2009-11-25
 *      Author: jinglei
 */

#include "MiningQueryLogHandler.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/tuple/tuple.hpp>
#include <log-manager/UserQuery.h>
#include <query-manager/QMCommonFunc.h>
#include <util/scheduler.h>
using namespace sf1r;


MiningQueryLogHandler::MiningQueryLogHandler()
        :recommendManagerList_(),collectionSet_(),waitSec_(3600), days_(7)
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
    recommendManagerList_.insert( std::make_pair(name, recommendManager) );
    collectionSet_.insert(name);
}

void MiningQueryLogHandler::runEvents()
{
    boost::unique_lock<boost::mutex> lock(mtx_, boost::try_to_lock);
    if (!lock.owns_lock())
    {
        std::cout<<"MiningQueryLogHandler locks"<<std::endl;
        return;
    }
    std::cout<<"[Start to update query information]"<<std::endl;
    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
    //collection specific feature: query recommend, query reminder, query correction

    //for query recommend
    map_it_type mlit = recommendManagerList_.begin();
    for ( ; mlit!=recommendManagerList_.end(); ++mlit)
    {
        mlit->second->RebuildForAll();
    }

    std::cout<<"[Updated query information]"<<std::endl;
}

bool MiningQueryLogHandler::cronStart(const std::string& cron_job)
{
    std::cout<<"MiningQueryLogHandler cron starting : "<<cron_job<<std::endl;
    if ( !cron_expression_.setExpression(cron_job) )
    {
        return false;
    }
    boost::function<void (void)> task = boost::bind(&MiningQueryLogHandler::cronJob_,this);
    izenelib::util::Scheduler::addJob("MiningQueryLogHandler", 60*1000, 0, task);
    return true;
}

void MiningQueryLogHandler::cronJob_()
{
    if (cron_expression_.matches_now())
    {
        runEvents();
    }
}
