#include "GetRecommendWorker.h"

namespace sf1r
{

GetRecommendWorker::GetRecommendWorker(
    ItemCFManager& itemCFManager,
    CoVisitManager& coVisitManager
)
    : itemCFManager_(itemCFManager)
    , coVisitManager_(coVisitManager)
{
}

void GetRecommendWorker::recommendPurchase(
    const RecommendInputParam& inputParam,
    idmlib::recommender::RecommendItemVec& results
)
{
    itemCFManager_.recommend(inputParam.limit, inputParam.inputItemIds,
                             results, &inputParam.itemFilter);
}

void GetRecommendWorker::recommendPurchaseFromWeight(
    const RecommendInputParam& inputParam,
    idmlib::recommender::RecommendItemVec& results
)
{
    itemCFManager_.recommend(inputParam.limit, inputParam.itemWeightMap,
                             results, &inputParam.itemFilter);
}

void GetRecommendWorker::recommendVisit(
    const RecommendInputParam& inputParam,
    idmlib::recommender::RecommendItemVec& results
)
{
    coVisitManager_.getCoVisitation(inputParam.limit, inputParam.inputItemIds[0],
                                    results, &inputParam.itemFilter);
}

void GetRecommendWorker::getCandidateSet(
    const std::vector<itemid_t>& inputItemIds,
    std::set<itemid_t>& candidateSet
)
{
    itemCFManager_.getCandidateSet(inputItemIds, candidateSet);
}

void GetRecommendWorker::recommendFromCandidateSet(
    const RecommendInputParam& inputParam,
    const std::set<itemid_t>& candidateSet,
    idmlib::recommender::RecommendItemVec& results
)
{
    itemCFManager_.recommendFromCandidateSet(inputParam.limit, inputParam.inputItemIds,
                                             candidateSet, results, &inputParam.itemFilter);
}

void GetRecommendWorker::getCandidateSetFromWeight(
    const ItemCFManager::ItemWeightMap& itemWeightMap,
    std::set<itemid_t>& candidateSet
)
{
    itemCFManager_.getCandidateSet(itemWeightMap, candidateSet);
}

void GetRecommendWorker::recommendFromCandidateSetWeight(
    const RecommendInputParam& inputParam,
    const std::set<itemid_t>& candidateSet,
    idmlib::recommender::RecommendItemVec& results
)
{
    itemCFManager_.recommendFromCandidateSet(inputParam.limit, inputParam.itemWeightMap,
                                             candidateSet, results, &inputParam.itemFilter);
}

} // namespace sf1r
