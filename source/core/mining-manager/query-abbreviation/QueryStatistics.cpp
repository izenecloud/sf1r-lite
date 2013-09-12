#include "QueryStatistics.h"
#include "RemoveKeywords.h"
#include "../MiningManager.h"
#include "mining-manager/suffix-match-manager/SuffixMatchManager.hpp"

#include <util/scheduler.h>
#include <glog/logging.h>
#include <log-manager/UserQuery.h>
#include <log-manager/LogAnalysis.h>

#include <node-manager/RequestLog.h>
#include <node-manager/DistributeFileSyncMgr.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/NodeManagerBase.h>
#include <node-manager/MasterManagerBase.h>

#include <iostream>
#include <time.h>
#include <string.h>
#include <stdlib.h>

namespace sf1r
{

static std::string cronJobName_ = "QueryStatistics";

QueryStatistics::QueryStatistics(MiningManager* mining, std::string& collectionName)
    : miningManager_(mining)
    , collectionName_(collectionName)
{
    wordsFreq_ = new FreqType;
    lastTimeStr_ = "20120100T000000";
    totalWords_ = 0;

    init();
    LOG(INFO)<<lastTimeStr_<<" "<<totalWords_;
    
    if (cronExpression_.setExpression("00 3 * * *"))
    {
        bool ret = izenelib::util::Scheduler::addJob(cronJobName_, 
                                                     60*1000,
                                                     0,
                                                     boost::bind(&QueryStatistics::cronJob_, this, _1));
        if (!ret)
        {
            LOG(INFO)<<"Failed to addJob:"<<cronJobName_;
        }
    }
}

QueryStatistics::~QueryStatistics()
{

    delete wordsFreq_;
    izenelib::util::Scheduler::removeJob(cronJobName_);
}

void QueryStatistics::init()
{
    std::ifstream ifs;
    std::string wordsFreqFile = miningManager_->system_resource_path_ + "/query-abbreviation/wordsFreq" + collectionName_;
    ifs.open(wordsFreqFile.c_str(), std::ifstream::in);
    if (ifs)
    {
        deserialize_(ifs);
        ifs.close();
    }
}

void QueryStatistics::serialize_(std::ostream& out)
{
    out<<lastTimeStr_<<"\n";
    out<<totalWords_<<"\n";
    FreqType::iterator it = wordsFreq_->begin();
    for (; it != wordsFreq_->end(); it++)
    {
        out<<it->first<<":"<<it->second<<"\n";
    }
}

void QueryStatistics::deserialize_(std::istream& in)
{
    if (!in)
        return;
    in>>lastTimeStr_;
    in>>totalWords_;
    char kv[512];
    while (in.getline(kv, 512))
    {
        std::string kvs(kv);
        std::size_t delim = kvs.find(":");
        if (std::string::npos == delim)
            continue;
        std::string k = kvs.substr(0, delim);
        
        unsigned long v = atol(kvs.substr(delim + 1).c_str());
        wordsFreq_->insert(make_pair(k, v));
        memset(kv, 0, 512);
    }
}

void QueryStatistics::cronJob_(int calltype)
{
    if (cronExpression_.matches_now() || calltype > 0)
    {
        if(calltype == 0 && NodeManagerBase::get()->isDistributed())
        {
            if (NodeManagerBase::get()->isPrimary())
            {
                MasterManagerBase::get()->pushWriteReq(cronJobName_, "cron");
                LOG(INFO) << "push cron job to queue on primary : " << cronJobName_;
            }
            else
                LOG(INFO) << "cron job ignored on replica: " << cronJobName_;
            return;
        }
        DISTRIBUTE_WRITE_BEGIN;
        DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;

        CronJobReqLog reqlog;
        if (!DistributeRequestHooker::get()->prepare(Req_CronJob, reqlog))
        {
            LOG(ERROR) << "!!!! prepare log failed while running cron job. : " << cronJobName_ << std::endl;
            return;
        }

        bool ret = statistics_();
        if (ret)
        {
            std::ofstream ofs;
            std::string wordsFreqFile = miningManager_->system_resource_path_ + "/query-abbreviation/wordsFreq" + collectionName_;
            if (!boost::filesystem::exists(wordsFreqFile))
            {
                boost::filesystem::create_directory(miningManager_->system_resource_path_ + "/query-abbreviation/");
            }
            ofs.open(wordsFreqFile.c_str(), std::ofstream::out | std::ofstream::trunc);
            serialize_(ofs);
            ofs.close();
        }
        DISTRIBUTE_WRITE_FINISH(true);
    }
}

bool QueryStatistics::statistics_()
{
    if (NULL == miningManager_)
        return false;
    std::vector<UserQuery> queries;
    LOG(INFO)<<"Begin time:"<<lastTimeStr_<<" Collection:"<<collectionName_;
    LogAnalysis::getRecentKeywordFreqList(collectionName_, lastTimeStr_, queries);
    time_t now = time(NULL);
    
    char last[64];
    memset(last, 0, 64);
    strftime(last, 64,"%Y%2m%2dT%2H%2M%2s", localtime(&now));
    lastTimeStr_ = last;

    LOG(INFO)<<queries.size();
    boost::unique_lock<boost::shared_mutex> ul(mtx_);
    for (std::size_t i = 0; i < queries.size(); i++)
    {
        const std::string& query = queries[i].getQuery();
        const uint32_t count = queries[i].getCount();
        RK::TokenArray tokens;
        RK::generateTokens(tokens, query, *miningManager_);
        
        for (std::size_t i = 0; i < tokens.size(); i++)
        {
            const std::string& keyword = tokens[i].token();
            
            FreqType::iterator it = wordsFreq_->find(keyword);
            if (wordsFreq_->end() == it)
            {
                wordsFreq_->insert(make_pair(keyword, count));
            }
            else
            {
                it->second += count;
            }
            totalWords_ += count;
        }

        if (tokens.size() <= 1)
            continue;
        for (std::size_t i = 0; i < tokens.size() - 1; i++)
        {
            const std::string& keyword = tokens[i].token() + tokens[i+1].token();
            //double weight = tokens[i].weight() + tokens[i+1].weight();
            //std::cout<<keyword<<"\n"; 
            FreqType::iterator it = wordsFreq_->find(keyword);
            if (wordsFreq_->end() == it)
            {
                wordsFreq_->insert(make_pair(keyword, count));
            }
            else
            {
                it->second += count;
            }
        }
    }
    return true;
}

double QueryStatistics::frequency(std::string word)
{
    boost::shared_lock<boost::shared_mutex> sl(mtx_);
    FreqType::iterator it = wordsFreq_->find(word);
    if (it == wordsFreq_->end())
    {
        if (0 == totalWords_)
            return 0.001;
        return 1000 * 0.9 / (double)(totalWords_ + 1);
    }
    return 1000 * it->second / (double)(totalWords_ + 1);
}

bool QueryStatistics::isCombine(const std::string& lv, const std::string& rv)
{
    FreqType::iterator it = wordsFreq_->find(lv+rv);
    unsigned long cof = 0;
    if (wordsFreq_->end() != it)
    {
        cof += it->second;
    }
    it = wordsFreq_->find(rv+lv);
    if (wordsFreq_->end() != it)
    {
        cof += it->second;
    }
    if (0 == cof)
        return false;

    it = wordsFreq_->find(lv);
    if (wordsFreq_->end() == it)
        return false;
    unsigned long lvf = it->second;
    //std::cout<<lv + rv <<" "<<cof <<" : "<<lv<<" "<<lvf<<"\n";
    return cof / (double)lvf > 0.25;
}

}
