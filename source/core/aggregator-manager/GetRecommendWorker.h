/**
 * @file GetRecommendWorker.h
 * @brief get recommendation result from worker
 * @author Jun Jiang
 * @date 2012-03-26
 */

#ifndef SF1R_GET_RECOMMEND_WORKER_H
#define SF1R_GET_RECOMMEND_WORKER_H

#include "GetRecommendBase.h"

namespace sf1r
{

class GetRecommendWorker : public GetRecommendBase
{
public:
    GetRecommendWorker(
        ItemCFManager& itemCFManager,
        CoVisitManager& coVisitManager
    );

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

private:
    ItemCFManager& itemCFManager_;
    CoVisitManager& coVisitManager_;
};

} // namespace sf1r

#endif // SF1R_GET_RECOMMEND_WORKER_H
