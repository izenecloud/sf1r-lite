#include "BOSRecommender.h"
#include "UserEventFilter.h"
#include "ItemFilter.h"
#include "CartManager.h"
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
    : ItemCFRecommender(itemManager, itemCFManager, userEventFilter)
    , cartManager_(cartManager)
{
}

bool BOSRecommender::recommend(RecommendParam& param, std::vector<RecommendItem>& recItemVec)
{
    if (!getCartItems_(param))
        return false;

    ItemFilter filter(itemManager_, param);
    if (param.userId && !userEventFilter_.filter(param.userId, param.inputItemIds, filter))
    {
        LOG(ERROR) << "failed to filter user event for user id " << param.userId;
        return false;
    }

    recommendItemCF_(param, filter, recItemVec);
    setReasonEvent_(recItemVec, RecommendSchema::CART_EVENT);

    return true;
}

bool BOSRecommender::getCartItems_(RecommendParam& param) const
{
    if (!param.inputItemIds.empty())
        return true;

    if (param.userId == 0)
    {
        LOG(ERROR) << "failed to recommend with empty user id and empty input items";
        return false;
    }

    if (!cartManager_.getCart(param.userId, param.inputItemIds))
    {
        LOG(ERROR) << "failed to get shopping cart items for user id " << param.userId;
        return false;
    }

    return true;
}

} // namespace sf1r
