/**
 * @file IndexAggregator.h
 * @author Zhongxia Li
 * @date Dec 5, 2011
 * @brief 
 */
#ifndef INDEX_AGGREGATOR_H_
#define INDEX_AGGREGATOR_H_

#include <net/aggregator/CallProxy.h>
#include <net/aggregator/Aggregator.h>

#include "IndexMerger.h"
#include "IndexWorker.h"

namespace sf1r
{

typedef net::aggregator::CallProxy<IndexMerger> IndexMergerProxy;

typedef net::aggregator::CallProxy<IndexWorker> IndexWorkerProxy;

typedef net::aggregator::Aggregator<IndexMergerProxy, IndexWorkerProxy> IndexAggregator;

} // namespace sf1r

#endif /* INDEX_AGGREGATOR_H_ */
