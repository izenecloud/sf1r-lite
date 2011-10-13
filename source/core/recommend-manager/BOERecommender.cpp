#include "BOERecommender.h"
#include "ItemFilter.h"
#include "UserEventFilter.h"

#include <glog/logging.h>

namespace sf1r
{

BOERecommender::BOERecommender(
    const RecommendSchema& schema,
    ItemCFManager* itemCFManager,
    const UserEventFilter& userEventFilter,
    userid_t userId,
    int maxRecNum,
    ItemFilter& filter
)
    : UserBaseRecommender(schema, itemCFManager, userEventFilter,
                          userId, maxRecNum, filter)
{
}

bool BOERecommender::recommend(std::vector<RecommendItem>& recItemVec)
{
    if (userId_ == 0)
    {
        LOG(ERROR) << "userId should be positive";
        return false;
    }

    UserEventFilter::ItemEventMap itemEventMap;
    std::vector<itemid_t> inputItemVec;

    if (! userEventFilter_.addUserEvent(userId_, itemEventMap, inputItemVec,
                                        filter_, notRecInputSet_))
    {
        LOG(ERROR) << "failed to add user event for user id " << userId_;
        return false;
    }

    recommendByItem_(inputItemVec, recItemVec);
    setReasonEvent_(recItemVec, itemEventMap);

    return true;
}

void BOERecommender::setReasonEvent_(
    std::vector<RecommendItem>& recItemVec,
    const UserEventFilter::ItemEventMap& itemEventMap
) const
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

} // namespace sf1r
