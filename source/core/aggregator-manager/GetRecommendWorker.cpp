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
    std::vector<itemid_t>& results
)
{
    coVisitManager_.getCoVisitation(inputParam.limit, inputParam.inputItemIds[0],
                                    results, &inputParam.itemFilter);
}

} // namespace sf1r
