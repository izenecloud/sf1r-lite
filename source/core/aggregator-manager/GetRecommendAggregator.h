/**
 * @file GetRecommendAggregator.h
 * @brief get recommendation result from aggregator
 * @author Jun Jiang
 * @date 2012-03-26
 */

#ifndef SF1R_GET_RECOMMEND_AGGREGATOR_H
#define SF1R_GET_RECOMMEND_AGGREGATOR_H

#include "GetRecommendBase.h"
#include "GetRecommendWorker.h"
#include <net/aggregator/Aggregator.h>

namespace sf1r
{

typedef net::aggregator::WorkerCaller<GetRecommendWorker> GetRecommendWorkerCaller;

class GetRecommendAggregator
    : public GetRecommendBase
    , protected net::aggregator::Aggregator<GetRecommendAggregator, GetRecommendWorkerCaller>
{
public:
    GetRecommendAggregator();

    virtual bool recommendPurchase(
        RecommendInputParam& inputParam,
        idmlib::recommender::RecommendItemVec& results
    );

    virtual bool recommendPurchaseFromWeight(
        RecommendInputParam& inputParam,
        idmlib::recommender::RecommendItemVec& results
    );

    virtual bool recommendVisit(
        RecommendInputParam& inputParam,
        std::vector<itemid_t>& results
    );
};

} // namespace sf1r

#endif // SF1R_GET_RECOMMEND_AGGREGATOR_H
