#include "QueryStatistics.h"
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
    bool ret = izenelib::util::Scheduler::addJob(cronJobName, 
                                                 60*10000,
                                                 0,
                                                 boost::bind(&QueryStatistics::statistics, this, 0));
    if (!ret)
    {
        LOG(INFO)<<"Failed to addJob:"<<cronJobName;
    }
}

QueryStatistics::~QueryStatistics()
{
    std::ofstream ofs;
    std::string wordsFreqFile = miningManager_->system_resource_path_ + "/query-abbreviation/wordsFreq" + collectionName_;
    LOG(INFO)<<wordsFreqFile;
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
    LOG(INFO)<<wordsFreqFile;
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
    if (NULL == miningManager_)
        return;
    std::vector<UserQuery> queries;
    LOG(INFO)<<"Begin time:"<<lastTimeStr_<<" Collection:"<<collectionName_;
    //LogAnalysis::getRecentKeywordFreqList(collectionName_, lastTimeStr_, queries);
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
        std::list<std::pair<UString, double> > major_tokens;
        std::list<std::pair<UString, double> > minor_tokens;
        izenelib::util::UString analyzedQuery;
        double rank_boundary = 0;
        miningManager_->getSuffixManager()->GetTokenResults(query, major_tokens, minor_tokens, analyzedQuery, rank_boundary);
        
        std::list<std::pair<UString, double> >::iterator it = major_tokens.begin();
        for (; it != major_tokens.end(); it++)
        {
            std::string keyword;
            it->first.convertString(keyword, izenelib::util::UString::UTF_8);
            FreqType::iterator it = wordsFreq_->find(keyword);
            if (wordsFreq_->end() == it)
            {
                wordsFreq_->insert(make_pair(keyword, 1));
            }
            else
            {
                (it->second)++;
            }
            totalWords_++;
        }
        for (it = minor_tokens.begin(); it != minor_tokens.end(); it++)
        {
            std::string keyword;
            it->first.convertString(keyword, izenelib::util::UString::UTF_8);
            FreqType::iterator it = wordsFreq_->find(keyword);
            if (wordsFreq_->end() == it)
            {
                wordsFreq_->insert(make_pair(keyword, 1));
            }
            else
            {
                (it->second)++;
            }
            totalWords_++;
        }
    }
}

double QueryStatistics::frequency(std::string word)
{
    boost::shared_lock<boost::shared_mutex> sl(mtx_);
    FreqType::iterator it = wordsFreq_->find(word);
    if (it == wordsFreq_->end())
        return 1000 * 0.9 / (double)totalWords_;
    return 1000 * it->second / (double)totalWords_;
}

}
