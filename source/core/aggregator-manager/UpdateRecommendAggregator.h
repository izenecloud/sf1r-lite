/**
 * @file UpdateRecommendAggregator.h
 * @brief the aggregator sends request to update recommendation matrix in workers 
 * @author Jun Jiang
 * @date 2012-04-10
 */

#ifndef SF1R_UPDATE_RECOMMEND_AGGREGATOR_H
#define SF1R_UPDATE_RECOMMEND_AGGREGATOR_H

#include <net/aggregator/CallProxy.h>
#include <net/aggregator/Aggregator.h>

#include "UpdateRecommendMerger.h"
#include "UpdateRecommendWorker.h"

namespace sf1r
{

typedef net::aggregator::CallProxy<UpdateRecommendMerger> UpdateRecommendMergerProxy;

typedef net::aggregator::CallProxy<UpdateRecommendWorker> UpdateRecommendWorkerProxy;

typedef net::aggregator::Aggregator<UpdateRecommendMergerProxy, UpdateRecommendWorkerProxy> UpdateRecommendAggregator;

} // namespace sf1r

#endif // SF1R_UPDATE_RECOMMEND_AGGREGATOR_H
