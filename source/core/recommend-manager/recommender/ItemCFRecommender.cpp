#include "ItemCFRecommender.h"
#include "ItemFilter.h"
#include "../common/RecommendParam.h"
#include "../common/RecommendItem.h"

#include <glog/logging.h>

namespace sf1r
{

ItemCFRecommender::ItemCFRecommender(
    ItemManager& itemManager,
    ItemCFManager& itemCFManager
)
    : Recommender(itemManager)
    , itemCFManager_(itemCFManager)
{
}

bool ItemCFRecommender::recommendImpl_(
    RecommendParam& param,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    idmlib::recommender::RecommendItemVec results;
    itemCFManager_.recommend(param.limit, param.inputItemIds, results, &filter);
    recItemVec.insert(recItemVec.end(), results.begin(), results.end());

    return true;
}

bool ItemCFRecommender::recommendFromItemWeight_(
    int limit,
    const ItemWeightMap& itemWeightMap,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    idmlib::recommender::RecommendItemVec results;
    itemCFManager_.recommend(limit, itemWeightMap, results, &filter);
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
