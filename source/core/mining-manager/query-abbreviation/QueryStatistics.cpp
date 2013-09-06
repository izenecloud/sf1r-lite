#include "QueryStatistics.h"
#include "RemoveKeywords.h"
#include "../MiningManager.h"
#include "mining-manager/suffix-match-manager/SuffixMatchManager.hpp"

#include <util/scheduler.h>
#include <glog/logging.h>
#include <log-manager/UserQuery.h>
#include <log-manager/LogAnalysis.h>

#include <iostream>
#include <time.h>
#include <string.h>
#include <stdlib.h>

namespace sf1r
{

static std::string cronJobName = "QueryStatistics";

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
        bool ret = izenelib::util::Scheduler::addJob(cronJobName, 
                                                     60*1000,
                                                     0,
                                                     boost::bind(&QueryStatistics::statistics, this, _1));
        if (!ret)
        {
            LOG(INFO)<<"Failed to addJob:"<<cronJobName;
        }
    }
}

QueryStatistics::~QueryStatistics()
{
    std::ofstream ofs;
    std::string wordsFreqFile = miningManager_->system_resource_path_ + "/query-abbreviation/wordsFreq" + collectionName_;
    if (!boost::filesystem::exists(wordsFreqFile))
    {
        boost::filesystem::create_directory(miningManager_->system_resource_path_ + "/query-abbreviation/");
    }
    ofs.open(wordsFreqFile.c_str(), std::ofstream::out | std::ofstream::trunc);
    serialize(ofs);
    ofs.close();

    delete wordsFreq_;
    izenelib::util::Scheduler::removeJob(cronJobName);
}

void QueryStatistics::init()
{
    std::ifstream ifs;
    std::string wordsFreqFile = miningManager_->system_resource_path_ + "/query-abbreviation/wordsFreq" + collectionName_;
    ifs.open(wordsFreqFile.c_str(), std::ifstream::in);
    if (ifs)
    {
        deserialize(ifs);
        ifs.close();
    }
}

void QueryStatistics::serialize(std::ostream& out)
{
    out<<lastTimeStr_<<"\n";
    out<<totalWords_<<"\n";
    FreqType::iterator it = wordsFreq_->begin();
    for (; it != wordsFreq_->end(); it++)
    {
        out<<it->first<<":"<<it->second<<"\n";
    }
}

void QueryStatistics::deserialize(std::istream& in)
{
    if (!in)
        return;
    in>>lastTimeStr_;
    in>>totalWords_;
    char* kv = new char[512];
    while (in.getline(kv, 512))
    {
        std::string kvs(kv);
        std::size_t delim = kvs.find(":");
        if (std::string::npos == delim)
            continue;
        std::string k = kvs.substr(0, delim);
        unsigned long v = atol(kvs.substr(delim + 1).c_str());
        wordsFreq_->insert(make_pair(k,v));
        memset(kv, 0, 512);
    }
    delete[] kv;
}

void QueryStatistics::statistics(int callType)
{
    if ((!cronExpression_.matches_now()) )
    {
        return;
    }
    if (NULL == miningManager_)
        return;
    std::vector<UserQuery> queries;
    LOG(INFO)<<"Begin time:"<<lastTimeStr_<<" Collection:"<<collectionName_;
    LogAnalysis::getRecentKeywordFreqList(collectionName_, lastTimeStr_, queries);
    time_t now = time(NULL);
    
    char* last = new char[64];
    memset(last, 0, 64);
    strftime(last, 64,"%Y%2m%2dT%2H%2M%2s", localtime(&now));
    lastTimeStr_ = last;
    delete last;

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
                (it->second) += count;
            }
            totalWords_ += count;
        }

        if (tokens.size() <= 1)
            continue;
        for (std::size_t i = 0; i < tokens.size() - 1; i++)
        {
            const std::string& keyword = tokens[i].token() + tokens[i+1].token();
            //std::cout<<keyword<<"\n"; 
            FreqType::iterator it = wordsFreq_->find(keyword);
            if (wordsFreq_->end() == it)
            {
                wordsFreq_->insert(make_pair(keyword, count));
            }
            else
            {
                (it->second) += count;
            }
        }
    }
}

double QueryStatistics::frequency(std::string word)
{
    boost::shared_lock<boost::shared_mutex> sl(mtx_);
    FreqType::iterator it = wordsFreq_->find(word);
    if (it == wordsFreq_->end())
        return 1000 * 0.9 / (double)(totalWords_ + 1);
    return 1000 * it->second / (double)(totalWords_ + 1);
}

bool QueryStatistics::isCombine(const std::string& lv, const std::string& rv)
{
    FreqType::iterator it = wordsFreq_->find(lv+rv);
    if (wordsFreq_->end() == it)
    {
        it = wordsFreq_->find(rv+lv);
        if (wordsFreq_->end() == it)
            return false;
    }
    unsigned long cof = it->second * 2;
    
    it = wordsFreq_->find(lv);
    if (wordsFreq_->end() == it)
        return false;
    unsigned long lvf = it->second;
    std::cout<<lv <<" "<<lvf <<" : "<<lv+rv<<" "<<cof<<"\n"; 
    return cof / (double)lvf > 0.8;
}

}
