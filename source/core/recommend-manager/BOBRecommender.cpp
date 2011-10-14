#include "BOBRecommender.h"
#include "UserEventFilter.h"
#include "ItemFilter.h"
#include "VisitManager.h"
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
    : ItemCFRecommender(itemManager, itemCFManager, userEventFilter)
    , visitManager_(visitManager)
{
}

bool BOBRecommender::recommend(RecommendParam& param, std::vector<RecommendItem>& recItemVec)
{
    if (!getBrowseItems_(param))
        return false;

    ItemFilter filter(itemManager_, param);
    if (param.userId && !userEventFilter_.filter(param.userId, param.inputItemIds, filter))
    {
        LOG(ERROR) << "failed to filter user event for user id " << param.userId;
        return false;
    }

    recommendItemCF_(param, filter, recItemVec);
    setReasonEvent_(recItemVec, RecommendSchema::BROWSE_EVENT);

    return true;
}

bool BOBRecommender::getBrowseItems_(RecommendParam& param) const
{
    if (!param.inputItemIds.empty())
        return true;

    if (param.userId == 0 || param.sessionIdStr.empty())
    {
        LOG(ERROR) << "failed to recommend with empty user/session id and empty input items";
        return false;
    }

    VisitSession visitSession;
    if (! visitManager_.getVisitSession(param.userId, visitSession))
    {
        LOG(ERROR) << "failed to get visit session items for user id " << param.userId;
        return false;
    }

    if (visitSession.sessionId_ == param.sessionIdStr)
    {
        param.inputItemIds.assign(visitSession.itemSet_.begin(), visitSession.itemSet_.end());
    }

    return true;
}

} // namespace sf1r
