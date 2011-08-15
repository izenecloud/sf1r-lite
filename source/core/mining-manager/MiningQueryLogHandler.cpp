/*
 * QueryDataBase.cpp
 *
 *  Created on: 2009-11-25
 *      Author: jinglei
 */

#include "MiningQueryLogHandler.h"
#include "auto-fill-submanager/AutoFillSubManager.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
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

    //for autofill and query correction
    processCollectionIndependent_(now);
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

void MiningQueryLogHandler::processCollectionIndependent_(const boost::posix_time::ptime& nowTime)
{
    boost::gregorian::days dd(days_);
    boost::posix_time::ptime p = nowTime-dd;
    std::string time_string = boost::posix_time::to_iso_string(p);
    std::string query_sql = "select query, count(*) as freq from user_queries where TimeStamp >='"+time_string+"' group by query";
    std::list<std::map<std::string, std::string> > query_records;
    UserQuery::find_by_sql(query_sql, query_records);
    std::list<std::map<std::string, std::string> >::iterator it = query_records.begin();

    std::list<std::pair<izenelib::util::UString,uint32_t> > logItems;
    for ( ;it!=query_records.end();++it )
    {
        izenelib::util::UString uquery( (*it)["query"], izenelib::util::UString::UTF_8);
        if ( QueryUtility::isRestrictWord( uquery ) )
        {
            continue;
        }
        uint32_t freq = boost::lexical_cast<uint32_t>( (*it)["freq"] );
        logItems.push_back( std::make_pair(uquery, freq) );
    }

    std::vector<boost::shared_ptr<LabelManager> > label_manager_list;
    map_it_type mlit = recommendManagerList_.begin();
    for ( ; mlit!=recommendManagerList_.end(); ++mlit)
    {
        boost::shared_ptr<LabelManager> label_manager = mlit->second->GetLabelManager();
        if ( label_manager )
        {
            label_manager_list.push_back(label_manager);
        }
    }
    QueryCorrectionSubmanager::getInstance().updateCogramAndDict(logItems);
    AutoFillSubManager::get()->buildIndex(logItems, label_manager_list);
    
}





