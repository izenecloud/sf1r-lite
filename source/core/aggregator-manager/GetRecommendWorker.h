/**
 * @file GetRecommendWorker.h
 * @brief get recommendation result from worker
 * @author Jun Jiang
 * @date 2012-03-26
 */

#ifndef SF1R_GET_RECOMMEND_WORKER_H
#define SF1R_GET_RECOMMEND_WORKER_H

#include "GetRecommendBase.h"
#include <net/aggregator/BindCallProxyBase.h>

namespace sf1r
{

class GetRecommendWorker : public GetRecommendBase
                         , public net::aggregator::BindCallProxyBase<GetRecommendWorker>
{
public:
    GetRecommendWorker(
        ItemCFManager& itemCFManager,
        CoVisitManager& coVisitManager
    );

    virtual bool bindCallProxy(CallProxyType& proxy)
    {
        BIND_CALL_PROXY_BEGIN(GetRecommendWorker, proxy)
        BIND_CALL_PROXY_2(
            recommendPurchase,
            RecommendInputParam,
            idmlib::recommender::RecommendItemVec
        )
        BIND_CALL_PROXY_2(
            recommendPurchaseFromWeight,
            RecommendInputParam,
            idmlib::recommender::RecommendItemVec
        )
        BIND_CALL_PROXY_2(
            recommendVisit,
            RecommendInputParam,
            idmlib::recommender::RecommendItemVec
        )
        BIND_CALL_PROXY_END()
    }

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
    ItemCFManager& itemCFManager_;
    CoVisitManager& coVisitManager_;
};

} // namespace sf1r

#endif // SF1R_GET_RECOMMEND_WORKER_H
