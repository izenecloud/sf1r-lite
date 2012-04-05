/**
 * @file SearchAggregator.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief index and minging search aggregator
 */
#ifndef SEARCH_AGGREGATOR_H_
#define SEARCH_AGGREGATOR_H_

#include <net/aggregator/CallProxy.h>
#include <net/aggregator/Aggregator.h>

#include "SearchMerger.h"
#include "SearchWorker.h"

namespace sf1r
{

typedef net::aggregator::CallProxy<SearchMerger> SearchMergerProxy;

typedef net::aggregator::CallProxy<SearchWorker> SearchWorkerProxy;

typedef net::aggregator::Aggregator<SearchMergerProxy, SearchWorkerProxy> SearchAggregator;

} // namespace sf1r

#endif /* SEARCH_AGGREGATOR_H_ */
