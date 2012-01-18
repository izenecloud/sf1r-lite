#include "BOERecommender.h"
#include "ItemFilter.h"
#include "UserEventFilter.h"
#include "RecommendParam.h"
#include "RecommendItem.h"
#include <bundles/recommend/RecommendSchema.h>

#include <glog/logging.h>
#include <boost/lexical_cast.hpp>

namespace
{

using namespace sf1r;

const float RATE_WEIGHTS[] = { 1.0, // general user event
                              -1.5, // rate 1, hate
                              -1.0, // rate 2, dislike
                               0.5, // rate 3, OK
                               1.0, // rate 4, like
                               1.5};// rate 5, love

void setReasonEvent(
    UserEventFilter::ItemEventMap& itemEventMap,
    ItemRateMap& itemRateMap,
    std::vector<RecommendItem>& recItemVec
)
{
    for (std::vector<RecommendItem>::iterator recIt = recItemVec.begin();
        recIt != recItemVec.end(); ++recIt)
    {
        std::vector<ReasonItem>& reasonItems = recIt->reasonItems_;

        for (std::vector<ReasonItem>::iterator reasonIt = reasonItems.begin();
            reasonIt != reasonItems.end(); ++reasonIt)
        {
            itemid_t itemId = reasonIt->item_.getId();
            reasonIt->event_ = itemEventMap[itemId];

            if (reasonIt->event_ == RecommendSchema::RATE_EVENT)
            {
                reasonIt->value_ = lexical_cast<std::string>(itemRateMap[itemId]);
            }
        }
    }
}

}

namespace sf1r
{

BOERecommender::BOERecommender(
    ItemManager& itemManager,
    ItemCFManager& itemCFManager,
    const UserEventFilter& userEventFilter
)
    : ItemCFRecommender(itemManager, itemCFManager)
    , userEventFilter_(userEventFilter)
{
}

bool BOERecommender::recommendImpl_(
    RecommendParam& param,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    if (param.userIdStr.empty())
    {
        LOG(ERROR) << "BOE type recommend requires non-empty userId";
        return false;
    }

    UserEventFilter::ItemEventMap itemEventMap;
    ItemRateMap itemRateMap;
    if (! userEventFilter_.addUserEvent(param.userIdStr, param.inputItemIds,
                                        itemRateMap, itemEventMap, filter))
    {
        LOG(ERROR) << "failed to add user event for user id " << param.userIdStr;
        return false;
    }

    if (itemRateMap.empty())
    {
        if (! ItemCFRecommender::recommendImpl_(param, filter, recItemVec))
            return false;
    }
    else
    {
        ItemWeightMap itemWeightMap;
        convertItemWeight_(param.inputItemIds, itemRateMap, itemWeightMap);

        if (! recommendFromItemWeight_(param.limit, itemWeightMap, filter, recItemVec))
            return false;
    }

    setReasonEvent(itemEventMap, itemRateMap, recItemVec);

    return true;
}

void BOERecommender::convertItemWeight_(
    const std::vector<itemid_t>& inputItemIds,
    const ItemRateMap& itemRateMap,
    ItemWeightMap& itemWeightMap
) const
{
    for (std::vector<itemid_t>::const_iterator it = inputItemIds.begin();
        it != inputItemIds.end(); ++it)
    {
        itemWeightMap[*it] = RATE_WEIGHTS[0];
    }

    for (ItemRateMap::const_iterator it = itemRateMap.begin();
        it != itemRateMap.end(); ++it)
    {
        itemWeightMap[it->first] = RATE_WEIGHTS[it->second];
    }
}

} // namespace sf1r
