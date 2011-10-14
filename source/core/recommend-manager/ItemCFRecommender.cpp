#include "ItemCFRecommender.h"
#include "UserEventFilter.h"
#include "ItemFilter.h"

#include <glog/logging.h>

namespace sf1r
{

ItemCFRecommender::ItemCFRecommender(
    ItemManager& itemManager,
    ItemCFManager& itemCFManager,
    const UserEventFilter& userEventFilter
)
    : Recommender(itemManager)
    , itemCFManager_(itemCFManager)
    , userEventFilter_(userEventFilter)
{
}

void ItemCFRecommender::recommendItemCF_(
    RecommendParam& param,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    idmlib::recommender::RecommendItemVec results;
    itemCFManager_.recommend(param.limit, param.inputItemIds, results, &filter);
    recItemVec.insert(recItemVec.end(), results.begin(), results.end());
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
