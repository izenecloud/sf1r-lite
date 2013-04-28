/**
 * @file GetRecommendMaster.h
 * @brief get recommendation result from master
 * @author Jun Jiang
 * @date 2012-04-05
 */

#ifndef SF1R_GET_RECOMMEND_MASTER_H
#define SF1R_GET_RECOMMEND_MASTER_H

#include "GetRecommendBase.h"
#include "GetRecommendAggregator.h"

#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace sf1r
{
class GetRecommendWorker;
class RecommendShardStrategy;

class GetRecommendMaster : public GetRecommendBase
{
public:
    /**
     * @param collection collection name
     * @param localWorker it could be NULL if no local worker exists
     */
    GetRecommendMaster(
        const std::string& collection,
        RecommendShardStrategy* shardStrategy,
        GetRecommendWorker* localWorker
    );

    ~GetRecommendMaster();

    virtual void recommendPurchase(
        const RecommendInputParam& inputParam,
        idmlib::recommender::RecommendItemVec& results
    );

    virtual void recommendPurchaseFromWeight(
        const RecommendInputParam& inputParam,
        idmlib::recommender::RecommendItemVec& results
    );

    virtual void recommendVisit(
        const RecommendInputParam& inputParam,
        idmlib::recommender::RecommendItemVec& results
    );

private:
    const std::string collection_;
    boost::scoped_ptr<GetRecommendMerger> merger_;
    boost::shared_ptr<GetRecommendAggregator> aggregator_;
    RecommendShardStrategy* shardStrategy_;
};

} // namespace sf1r

#endif // SF1R_GET_RECOMMEND_MASTER_H
