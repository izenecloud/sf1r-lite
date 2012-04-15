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
    typedef std::set<itemid_t> CandidateSet;
    typedef idmlib::recommender::RecommendItemVec RecommendItemVec;

    virtual bool bindCallProxy(CallProxyType& proxy)
    {
        BIND_CALL_PROXY_BEGIN(GetRecommendMerger, proxy)
        BIND_CALL_PROXY_2(
            getCandidateSet,
            net::aggregator::WorkerResults<CandidateSet>,
            CandidateSet
        )
        BIND_CALL_PROXY_2(
            recommendFromCandidateSet,
            net::aggregator::WorkerResults<RecommendItemVec>,
            RecommendItemVec
        )
        BIND_CALL_PROXY_2(
            getCandidateSetFromWeight,
            net::aggregator::WorkerResults<CandidateSet>,
            CandidateSet
        )
        BIND_CALL_PROXY_2(
            recommendFromCandidateSetWeight,
            net::aggregator::WorkerResults<RecommendItemVec>,
            RecommendItemVec
        )
        BIND_CALL_PROXY_END()
    }

    void getCandidateSet(
        const net::aggregator::WorkerResults<CandidateSet>& workerResults,
        CandidateSet& mergeResult
    );

    void recommendFromCandidateSet(
        const net::aggregator::WorkerResults<RecommendItemVec>& workerResults,
        RecommendItemVec& mergeResult
    );

    void getCandidateSetFromWeight(
        const net::aggregator::WorkerResults<CandidateSet>& workerResults,
        CandidateSet& mergeResult
    );

    void recommendFromCandidateSetWeight(
        const net::aggregator::WorkerResults<RecommendItemVec>& workerResults,
        RecommendItemVec& mergeResult
    );

private:
    void mergeRecommendResult_(
        const net::aggregator::WorkerResults<RecommendItemVec>& workerResults,
        RecommendItemVec& mergeResult
    );

    void mergeCandidateSet_(
        const net::aggregator::WorkerResults<CandidateSet>& workerResults,
        CandidateSet& mergeResult
    );
};

} // namespace sf1r

#endif // SF1R_GET_RECOMMEND_MERGER_H
