/**
 * @file IndexAggregator.h
 * @author Zhongxia Li
 * @date Dec 5, 2011
 * @brief 
 */
#ifndef INDEX_AGGREGATOR_H_
#define INDEX_AGGREGATOR_H_

#include <net/aggregator/Typedef.h>
#include <net/aggregator/Aggregator.h>
#include <net/aggregator/AggregatorConfig.h>

#include "IndexWorker.h"

using namespace net::aggregator;

namespace sf1r
{

typedef WorkerCaller<IndexWorker> IndexWorkerCaller;

class IndexAggregator : public Aggregator<IndexAggregator, IndexWorkerCaller>
{
public:
    IndexAggregator(IndexWorker* indexWorker)
    {
        localWorkerCaller_.reset(new IndexWorkerCaller(indexWorker));

        ADD_FUNC_TO_WORKER_CALLER(IndexWorkerCaller, localWorkerCaller_, IndexWorker, index);
    }


    bool ScdDispatch(
            unsigned int numdoc,
            const std::string& collectionName,
            const std::string& scdPath);

    bool aggregate(const std::string& func, bool& ret, const std::vector<std::pair<workerid_t, bool> >& resultList)
    {
        if (func == "index")
        {
            ret = true;
            return ret;
        }
        return false;
    }
};

}

#endif /* INDEX_AGGREGATOR_H_ */
