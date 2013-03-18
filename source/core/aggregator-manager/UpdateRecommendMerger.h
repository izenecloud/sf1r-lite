/**
 * @file UpdateRecommendMerger.h
 * @brief merge update results of recommendation matrix
 * @author Jun Jiang
 * @date 2012-04-10
 */

#ifndef SF1R_UPDATE_RECOMMEND_MERGER_H
#define SF1R_UPDATE_RECOMMEND_MERGER_H

#include <net/aggregator/BindCallProxyBase.h>
#include <net/aggregator/WorkerResults.h>

namespace sf1r
{

class UpdateRecommendMerger : public net::aggregator::BindCallProxyBase<UpdateRecommendMerger>
{
public:
    virtual bool bindCallProxy(CallProxyType& proxy)
    {
        BIND_CALL_PROXY_BEGIN(UpdateRecommendMerger, proxy)
        BIND_CALL_PROXY_2(
            updatePurchaseMatrix,
            net::aggregator::WorkerResults<bool>,
            bool
        )
        BIND_CALL_PROXY_2(
            updatePurchaseCoVisitMatrix,
            net::aggregator::WorkerResults<bool>,
            bool
        )
        BIND_CALL_PROXY_2(
            buildPurchaseSimMatrix,
            net::aggregator::WorkerResults<bool>,
            bool
        )
        BIND_CALL_PROXY_2(
            updateVisitMatrix,
            net::aggregator::WorkerResults<bool>,
            bool
        )
        BIND_CALL_PROXY_2(
            flushRecommendMatrix,
            net::aggregator::WorkerResults<bool>,
            bool
        )
        BIND_CALL_PROXY_2(HookDistributeRequestForUpdateRec, net::aggregator::WorkerResults<bool>, bool)
        BIND_CALL_PROXY_END()
    }

    void updatePurchaseMatrix(
        const net::aggregator::WorkerResults<bool>& workerResults,
        bool& mergeResult
    );

    void updatePurchaseCoVisitMatrix(
        const net::aggregator::WorkerResults<bool>& workerResults,
        bool& mergeResult
    );

    void buildPurchaseSimMatrix(
        const net::aggregator::WorkerResults<bool>& workerResults,
        bool& mergeResult
    );

    void updateVisitMatrix(
        const net::aggregator::WorkerResults<bool>& workerResults,
        bool& mergeResult
    );

    void flushRecommendMatrix(
        const net::aggregator::WorkerResults<bool>& workerResults,
        bool& mergeResult
    );

    void HookDistributeRequestForUpdateRec(const net::aggregator::WorkerResults<bool>& workerResults, bool& mergeResult);
private:
    void mergeUpdateResult_(
        const net::aggregator::WorkerResults<bool>& workerResults,
        bool& mergeResult
    );
};

} // namespace sf1r

#endif // SF1R_UPDATE_RECOMMEND_MERGER_H
