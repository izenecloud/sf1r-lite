#include "ItemCFRecommender.h"

#include <glog/logging.h>

namespace sf1r
{

ItemCFRecommender::ItemCFRecommender(ItemCFManager& itemCFManager)
    : itemCFManager_(itemCFManager)
{
}

bool ItemCFRecommender::recommendImpl_(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    idmlib::recommender::RecommendItemVec results;
    itemCFManager_.recommend(param.inputParam.limit, param.inputParam.inputItemIds,
                             results, &param.inputParam.itemFilter);
    recItemVec.insert(recItemVec.end(), results.begin(), results.end());

    return true;
}

bool ItemCFRecommender::recommendFromItemWeight_(
    RecommendInputParam& inputParam,
    std::vector<RecommendItem>& recItemVec
)
{
    idmlib::recommender::RecommendItemVec results;
    itemCFManager_.recommend(inputParam.limit, inputParam.itemWeightMap,
                             results, &inputParam.itemFilter);
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
