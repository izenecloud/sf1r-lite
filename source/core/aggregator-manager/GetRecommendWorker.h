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
#include <util/osgi/IService.h>

namespace sf1r
{

class GetRecommendWorker : public GetRecommendBase
                         , public net::aggregator::BindCallProxyBase<GetRecommendWorker>
                         , public ::izenelib::osgi::IService
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
            recommendVisit,
            RecommendInputParam,
            idmlib::recommender::RecommendItemVec
        )
        BIND_CALL_PROXY_2(
            getCandidateSet,
            std::vector<itemid_t>,
            std::set<itemid_t>
        )
        BIND_CALL_PROXY_3(
            recommendFromCandidateSet,
            RecommendInputParam,
            std::set<itemid_t>,
            idmlib::recommender::RecommendItemVec
        )
        BIND_CALL_PROXY_2(
            getCandidateSetFromWeight,
            ItemCFManager::ItemWeightMap,
            std::set<itemid_t>
        )
        BIND_CALL_PROXY_3(
            recommendFromCandidateSetWeight,
            RecommendInputParam,
            std::set<itemid_t>,
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

    void getCandidateSet(
        const std::vector<itemid_t>& inputItemIds,
        std::set<itemid_t>& candidateSet
    );

    void recommendFromCandidateSet(
        const RecommendInputParam& inputParam,
        const std::set<itemid_t>& candidateSet,
        idmlib::recommender::RecommendItemVec& results
    );

    void getCandidateSetFromWeight(
        const ItemCFManager::ItemWeightMap& itemWeightMap,
        std::set<itemid_t>& candidateSet
    );

    void recommendFromCandidateSetWeight(
        const RecommendInputParam& inputParam,
        const std::set<itemid_t>& candidateSet,
        idmlib::recommender::RecommendItemVec& results
    );

private:
    ItemCFManager& itemCFManager_;
    CoVisitManager& coVisitManager_;
};

} // namespace sf1r

#endif // SF1R_GET_RECOMMEND_WORKER_H
