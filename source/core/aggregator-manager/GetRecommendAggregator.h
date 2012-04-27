/**
 * @file GetRecommendAggregator.h
 * @brief get recommendation result from aggregator
 * @author Jun Jiang
 * @date 2012-03-26
 */

#ifndef SF1R_GET_RECOMMEND_AGGREGATOR_H
#define SF1R_GET_RECOMMEND_AGGREGATOR_H

#include <net/aggregator/CallProxy.h>
#include <net/aggregator/Aggregator.h>

#include "GetRecommendMerger.h"
#include "GetRecommendWorker.h"

namespace sf1r
{

typedef net::aggregator::CallProxy<GetRecommendMerger> GetRecommendMergerProxy;

typedef net::aggregator::CallProxy<GetRecommendWorker> GetRecommendWorkerProxy;

typedef net::aggregator::Aggregator<GetRecommendMergerProxy, GetRecommendWorkerProxy> GetRecommendAggregator;

} // namespace sf1r

#endif // SF1R_GET_RECOMMEND_AGGREGATOR_H
