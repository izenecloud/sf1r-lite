/*
 * QueryRecommendSubmanager.cpp
 *
 *  Created on: 2009-6-10
 *      Author: jinglei
 *  Modified on: 2009-06-15
 *       Author: Liang
 *  Refined on: 2009-11-05
 *       Author: Jia
 *  Refined on: 2009-11-24
 *       Author: Jinglei
 *
 */

#include "QueryRecommendSubmanager.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <am/3rdparty/rde_hash.h>
#include <boost/timer.hpp>
using namespace sf1r;

QueryRecommendSubmanager::QueryRecommendSubmanager(const boost::shared_ptr<RecommendManager>& rmDb) :
    rmDb_(rmDb)
{

}

QueryRecommendSubmanager::~QueryRecommendSubmanager()
{

}


void QueryRecommendSubmanager::getRecommendQuery(const izenelib::util::UString& query,
        const std::vector<docid_t>& topDocIdList, unsigned int maxNum,
        QueryRecommendRep& recommendRep)
{
    rmDb_->getRelatedConcepts(query, maxNum, recommendRep.recommendQueries_);
    recommendRep.scores_.resize(recommendRep.recommendQueries_.size());
    for(uint32_t i=0;i<recommendRep.scores_.size();i++)
    {
        recommendRep.scores_[i] = 1.0;
    }

}


bool QueryRecommendSubmanager::getReminderQuery(std::vector<izenelib::util::UString>& popularQueries, std::vector<izenelib::util::UString>& realTimeQueries)
{
    try
    {
        rmDb_->getReminderQuery(popularQueries, realTimeQueries);
    }
    catch( std::exception& ex)
    {
        
        return false;
    }
    return true;
}

