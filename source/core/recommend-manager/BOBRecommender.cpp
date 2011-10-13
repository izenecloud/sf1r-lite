#include "BOBRecommender.h"
#include "ItemFilter.h"
#include "VisitManager.h"
#include <bundles/recommend/RecommendSchema.h>

#include <glog/logging.h>

namespace sf1r
{

BOBRecommender::BOBRecommender(
    const RecommendSchema& schema,
    ItemCFManager* itemCFManager,
    const UserEventFilter& userEventFilter,
    userid_t userId,
    int maxRecNum,
    ItemFilter& filter,
    VisitManager* visitManager
)
    : UserBaseRecommender(schema, itemCFManager, userEventFilter,
                          userId, maxRecNum, filter)
    , visitManager_(visitManager)
{
}

bool BOBRecommender::recommend(
    const std::string& sessionIdStr,
    const std::vector<itemid_t>& inputItemVec,
    std::vector<RecommendItem>& recItemVec
)
{
    if (inputItemVec.empty() == false)
    {
        // use input items as browse history
        return recommendImpl_(inputItemVec, recItemVec);
    }

    std::vector<itemid_t> browseItemVec;
    if (getBrowseItems_(sessionIdStr, browseItemVec))
    {
        return recommendImpl_(browseItemVec, recItemVec);
    }

    return false;
}

bool BOBRecommender::recommendImpl_(
    const std::vector<itemid_t>& inputItemVec,
    std::vector<RecommendItem>& recItemVec
)
{
    if (filterUserEvent_() == false)
    {
        LOG(ERROR) << "failed to filter user event for user id " << userId_;
        return false;
    }

    recommendByItem_(inputItemVec, recItemVec);
    setReasonEvent_(recItemVec, RecommendSchema::BROWSE_EVENT);

    return true;
}

bool BOBRecommender::getBrowseItems_(
    const std::string& sessionIdStr,
    std::vector<itemid_t>& browseItemVec
) const
{
    if (userId_ == 0 || sessionIdStr.empty())
    {
        LOG(ERROR) << "failed to recommend with empty browse history";
        return false;
    }

    VisitSession visitSession;
    if (visitManager_->getVisitSession(userId_, visitSession) == false)
    {
        LOG(ERROR) << "failed to get visit session items for user id " << userId_;
        return false;
    }

    if (visitSession.sessionId_ == sessionIdStr)
    {
        browseItemVec.assign(visitSession.itemSet_.begin(), visitSession.itemSet_.end());
    }

    return true;
}

} // namespace sf1r
