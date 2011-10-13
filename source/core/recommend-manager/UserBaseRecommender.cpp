#include "UserBaseRecommender.h"
#include "ItemFilter.h"
#include "UserEventFilter.h"
#include <bundles/recommend/RecommendSchema.h>

#include <glog/logging.h>

namespace sf1r
{

UserBaseRecommender::UserBaseRecommender(
    const RecommendSchema& schema,
    ItemCFManager* itemCFManager,
    const UserEventFilter& userEventFilter,
    userid_t userId,
    int maxRecNum,
    ItemFilter& filter
)
    : recommendSchema_(schema)
    , itemCFManager_(itemCFManager)
    , userEventFilter_(userEventFilter)
    , userId_(userId)
    , maxRecNum_(maxRecNum)
    , filter_(filter)
{
}

void UserBaseRecommender::recommendByItem_(
    const std::vector<itemid_t>& inputItemVec,
    std::vector<RecommendItem>& recItemVec
)
{
    if (notRecInputSet_.empty())
    {
        recommendByItemImpl_(inputItemVec, recItemVec);
    }
    else
    {
        // exclude "notRecInputSet" in input items
        std::vector<itemid_t> newInputItemVec;
        for (std::vector<itemid_t>::const_iterator it = inputItemVec.begin(); 
            it != inputItemVec.end(); ++it)
        {
            if (notRecInputSet_.find(*it) == notRecInputSet_.end())
            {
                newInputItemVec.push_back(*it);
            }
        }

        recommendByItemImpl_(newInputItemVec, recItemVec);
    }
}

void UserBaseRecommender::recommendByItemImpl_(
    const std::vector<itemid_t>& inputItemVec,
    std::vector<RecommendItem>& recItemVec
)
{
    idmlib::recommender::RecommendItemVec results;
    itemCFManager_->recommend(maxRecNum_, inputItemVec, results, &filter_);
    recItemVec.insert(recItemVec.end(), results.begin(), results.end());
}

bool UserBaseRecommender::filterUserEvent_()
{
    if (userId_ == 0)
        return true;

    return userEventFilter_.filter(userId_, filter_, notRecInputSet_);
}

void UserBaseRecommender::setReasonEvent_(
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
