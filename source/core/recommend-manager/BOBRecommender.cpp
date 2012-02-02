#include "BOBRecommender.h"
#include "UserEventFilter.h"
#include "storage/VisitManager.h"
#include "RecommendParam.h"
#include <bundles/recommend/RecommendSchema.h>

#include <glog/logging.h>

namespace sf1r
{

BOBRecommender::BOBRecommender(
    ItemManager& itemManager,
    ItemCFManager& itemCFManager,
    const UserEventFilter& userEventFilter,
    VisitManager& visitManager
)
    : ItemCFRecommender(itemManager, itemCFManager)
    , userEventFilter_(userEventFilter)
    , visitManager_(visitManager)
{
}

bool BOBRecommender::recommendImpl_(
    RecommendParam& param,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    if (! getBrowseItems_(param))
        return false;

    if (!param.userIdStr.empty() &&
        !userEventFilter_.filter(param.userIdStr, param.inputItemIds, filter))
    {
        LOG(ERROR) << "failed to filter user event for user id " << param.userIdStr;
        return false;
    }

    if (! ItemCFRecommender::recommendImpl_(param, filter, recItemVec))
        return false;

    setReasonEvent_(recItemVec, RecommendSchema::BROWSE_EVENT);

    return true;
}

bool BOBRecommender::getBrowseItems_(RecommendParam& param) const
{
    if (! param.inputItemIds.empty())
        return true;

    if (param.userIdStr.empty() ||
        param.sessionIdStr.empty())
    {
        LOG(ERROR) << "failed to recommend with empty user/session id and empty input items";
        return false;
    }

    VisitSession visitSession;
    if (! visitManager_.getVisitSession(param.userIdStr, visitSession))
    {
        LOG(ERROR) << "failed to get visit session items for user id " << param.userIdStr;
        return false;
    }

    if (visitSession.sessionId_ == param.sessionIdStr)
    {
        param.inputItemIds.assign(visitSession.itemSet_.begin(), visitSession.itemSet_.end());
    }

    return true;
}

} // namespace sf1r
