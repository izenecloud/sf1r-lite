/**
 * @file GetRecommendMerger.h
 * @brief merge recommendation result
 * @author Jun Jiang
 * @date 2012-04-05
 */

#ifndef SF1R_GET_RECOMMEND_MERGER_H
#define SF1R_GET_RECOMMEND_MERGER_H

#include <net/aggregator/BindCallProxyBase.h>
#include <net/aggregator/WorkerResults.h>
#include <recommend-manager/common/RecTypes.h>

namespace sf1r
{

class GetRecommendMerger : public net::aggregator::BindCallProxyBase<GetRecommendMerger>
{
public:
    typedef idmlib::recommender::RecommendItemVec RecommendItemVec;

    virtual bool bindCallProxy(CallProxyType& proxy)
    {
        BIND_CALL_PROXY_BEGIN(GetRecommendMerger, proxy)
        BIND_CALL_PROXY_2(
            recommendPurchase,
            net::aggregator::WorkerResults<RecommendItemVec>,
            RecommendItemVec
        )
        BIND_CALL_PROXY_2(
            recommendPurchaseFromWeight,
            net::aggregator::WorkerResults<RecommendItemVec>,
            RecommendItemVec
        )
        BIND_CALL_PROXY_2(
            recommendVisit,
            net::aggregator::WorkerResults<RecommendItemVec>,
            RecommendItemVec
        )
        BIND_CALL_PROXY_END()
    }

    void recommendPurchase(
        const net::aggregator::WorkerResults<RecommendItemVec>& workerResults,
        RecommendItemVec& mergeResult
    );

    void recommendPurchaseFromWeight(
        const net::aggregator::WorkerResults<RecommendItemVec>& workerResults,
        RecommendItemVec& mergeResult
    );

    void recommendVisit(
        const net::aggregator::WorkerResults<RecommendItemVec>& workerResults,
        RecommendItemVec& mergeResult
    );

private:
    void mergeRecommendResult_(
        const net::aggregator::WorkerResults<RecommendItemVec>& workerResults,
        RecommendItemVec& mergeResult
    );
};

} // namespace sf1r

#endif // SF1R_GET_RECOMMEND_MERGER_H
