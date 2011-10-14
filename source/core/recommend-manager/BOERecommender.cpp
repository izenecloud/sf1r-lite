#include "BOERecommender.h"
#include "ItemFilter.h"
#include "UserEventFilter.h"

#include <glog/logging.h>

namespace
{

using namespace sf1r;

void setReasonEvent(
    std::vector<RecommendItem>& recItemVec,
    const UserEventFilter::ItemEventMap& itemEventMap
)
{
    for (std::vector<RecommendItem>::iterator recIt = recItemVec.begin();
        recIt != recItemVec.end(); ++recIt)
    {
        std::vector<ReasonItem>& reasonItems = recIt->reasonItems_;
        for (std::vector<ReasonItem>::iterator reasonIt = reasonItems.begin();
            reasonIt != reasonItems.end(); ++reasonIt)
        {
            UserEventFilter::ItemEventMap::const_iterator findIt = itemEventMap.find(reasonIt->itemId_);
            if (findIt != itemEventMap.end())
            {
                reasonIt->event_ = findIt->second;
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
    : ItemCFRecommender(itemManager, itemCFManager, userEventFilter)
{
}

bool BOERecommender::recommend(RecommendParam& param, std::vector<RecommendItem>& recItemVec)
{
    if (param.userId == 0)
    {
        LOG(ERROR) << "BOE type recommend requires non-empty userId";
        return false;
    }

    ItemFilter filter(itemManager_, param);
    UserEventFilter::ItemEventMap itemEventMap;
    if (! userEventFilter_.addUserEvent(param.userId, param.inputItemIds,
                                        itemEventMap, filter))
    {
        LOG(ERROR) << "failed to add user event for user id " << param.userId;
        return false;
    }

    recommendItemCF_(param, filter, recItemVec);
    setReasonEvent(recItemVec, itemEventMap);

    return true;
}

} // namespace sf1r
