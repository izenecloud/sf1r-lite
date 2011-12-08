/**
 * @file RecommendAggregator.h
 * @author Zhongxia Li
 * @date Dec 8, 2011 initial creatation
 * @brief 
 */
#ifndef RECOMMEND_AGGREGATOR_H_
#define RECOMMEND_AGGREGATOR_H_

#include <net/aggregator/Typedef.h>
#include <net/aggregator/Aggregator.h>
#include <net/aggregator/AggregatorConfig.h>

#include "RecommendWorker.h"

using namespace net::aggregator;

namespace sf1r
{

typedef WorkerCaller<RecommendWorker> RecommendWorkerCaller;

class RecommendAggregator : public Aggregator<RecommendAggregator, RecommendWorkerCaller>
{
public:
    RecommendAggregator(RecommendWorker* recommendWorker)
    {
        localWorkerCaller_.reset(new RecommendWorkerCaller(recommendWorker));

        //ADD_FUNC_TO_WORKER_CALLER(RecommendWorkerCaller, localWorkerCaller_, RecommendWorker, XXX);
    }
};

}

#endif /* RECOMMEND_AGGREGATOR_H_ */
