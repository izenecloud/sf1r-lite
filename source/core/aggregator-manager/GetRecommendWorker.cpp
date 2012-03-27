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

bool GetRecommendWorker::recommendPurchase(
    RecommendInputParam& inputParam,
    idmlib::recommender::RecommendItemVec& results
)
{
    itemCFManager_.recommend(inputParam.limit, inputParam.inputItemIds,
                             results, &inputParam.itemFilter);
    return true;
}

bool GetRecommendWorker::recommendPurchaseFromWeight(
    RecommendInputParam& inputParam,
    idmlib::recommender::RecommendItemVec& results
)
{
    itemCFManager_.recommend(inputParam.limit, inputParam.itemWeightMap,
                             results, &inputParam.itemFilter);
    return true;
}

bool GetRecommendWorker::recommendVisit(
    RecommendInputParam& inputParam,
    std::vector<itemid_t>& results
)
{
    coVisitManager_.getCoVisitation(inputParam.limit, inputParam.inputItemIds[0],
                                    results, &inputParam.itemFilter);
    return true;
}

} // namespace sf1r
