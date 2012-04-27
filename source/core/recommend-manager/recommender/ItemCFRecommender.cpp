#include "ItemCFRecommender.h"

#include <glog/logging.h>

namespace sf1r
{

ItemCFRecommender::ItemCFRecommender(GetRecommendBase& getRecommendBase)
    : getRecommendBase_(getRecommendBase)
{
}

bool ItemCFRecommender::recommendImpl_(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    if (param.inputParam.inputItemIds.empty())
        return true;

    idmlib::recommender::RecommendItemVec results;
    getRecommendBase_.recommendPurchase(param.inputParam, results);
    recItemVec.insert(recItemVec.end(), results.begin(), results.end());

    return true;
}

bool ItemCFRecommender::recommendFromItemWeight_(
    RecommendInputParam& inputParam,
    std::vector<RecommendItem>& recItemVec
)
{
    if (inputParam.itemWeightMap.empty())
        return true;

    idmlib::recommender::RecommendItemVec results;
    getRecommendBase_.recommendPurchaseFromWeight(inputParam, results);
    recItemVec.insert(recItemVec.end(), results.begin(), results.end());

    return true;
}

void ItemCFRecommender::setReasonEvent_(
    std::vector<RecommendItem>& recItemVec,
    const std::string& eventName
) const
{
    for (std::vector<RecommendItem>::iterator recIt = recItemVec.begin();
        recIt != recItemVec.end(); ++recIt)
    {
        for (std::vector<ReasonItem>::iterator reasonIt = recIt->reasonItems_.begin();
            reasonIt != recIt->reasonItems_.end(); ++reasonIt)
        {
            reasonIt->event_ = eventName;
        }
    }
}

} // namespace sf1r
