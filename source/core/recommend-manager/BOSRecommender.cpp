#include "BOSRecommender.h"
#include "UserEventFilter.h"
#include "CartManager.h"
#include "RecommendParam.h"
#include <bundles/recommend/RecommendSchema.h>

#include <glog/logging.h>

namespace sf1r
{

BOSRecommender::BOSRecommender(
    ItemManager& itemManager,
    ItemCFManager& itemCFManager,
    const UserEventFilter& userEventFilter,
    CartManager& cartManager
)
    : ItemCFRecommender(itemManager, itemCFManager)
    , userEventFilter_(userEventFilter)
    , cartManager_(cartManager)
{
}

bool BOSRecommender::recommendImpl_(
    RecommendParam& param,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    if (! getCartItems_(param))
        return false;

    if (!param.userIdStr.empty() &&
        !userEventFilter_.filter(param.userIdStr, param.inputItemIds, filter))
    {
        LOG(ERROR) << "failed to filter user event for user id " << param.userIdStr;
        return false;
    }

    if (! ItemCFRecommender::recommendImpl_(param, filter, recItemVec))
        return false;

    setReasonEvent_(recItemVec, RecommendSchema::CART_EVENT);

    return true;
}

bool BOSRecommender::getCartItems_(RecommendParam& param) const
{
    if (! param.inputItemIds.empty())
        return true;

    if (param.userIdStr.empty())
    {
        LOG(ERROR) << "failed to recommend with empty user id and empty input items";
        return false;
    }

    if (!cartManager_.getCart(param.userIdStr, param.inputItemIds))
    {
        LOG(ERROR) << "failed to get shopping cart items for user id " << param.userIdStr;
        return false;
    }

    return true;
}

} // namespace sf1r
