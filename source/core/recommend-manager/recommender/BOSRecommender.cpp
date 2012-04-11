#include "BOSRecommender.h"
#include "UserEventFilter.h"
#include "../storage/CartManager.h"
#include <bundles/recommend/RecommendSchema.h>

#include <glog/logging.h>

namespace sf1r
{

BOSRecommender::BOSRecommender(
    GetRecommendBase& getRecommendBase,
    const UserEventFilter& userEventFilter,
    CartManager& cartManager
)
    : ItemCFRecommender(getRecommendBase)
    , userEventFilter_(userEventFilter)
    , cartManager_(cartManager)
{
}

bool BOSRecommender::recommendImpl_(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    if (! getCartItems_(param))
        return false;

    if (!param.userIdStr.empty() &&
        !userEventFilter_.filter(param.userIdStr, param.inputParam.inputItemIds, param.inputParam.itemFilter))
    {
        LOG(ERROR) << "failed to filter user event for user id " << param.userIdStr;
        return false;
    }

    if (! ItemCFRecommender::recommendImpl_(param, recItemVec))
        return false;

    setReasonEvent_(recItemVec, RecommendSchema::CART_EVENT);

    return true;
}

bool BOSRecommender::getCartItems_(RecommendParam& param) const
{
    if (! param.inputParam.inputItemIds.empty())
        return true;

    if (param.userIdStr.empty())
    {
        LOG(ERROR) << "failed to recommend with empty user id and empty input items";
        return false;
    }

    if (! cartManager_.getCart(param.userIdStr, param.inputParam.inputItemIds))
    {
        LOG(ERROR) << "failed to get shopping cart items for user id " << param.userIdStr;
        return false;
    }

    return true;
}

} // namespace sf1r
