#include "BOSRecommender.h"
#include "ItemFilter.h"
#include "CartManager.h"
#include <bundles/recommend/RecommendSchema.h>

#include <glog/logging.h>

namespace sf1r
{

BOSRecommender::BOSRecommender(
    const RecommendSchema& schema,
    ItemCFManager* itemCFManager,
    const UserEventFilter& userEventFilter,
    userid_t userId,
    int maxRecNum,
    ItemFilter& filter,
    CartManager* cartManager
)
    : UserBaseRecommender(schema, itemCFManager, userEventFilter,
                          userId, maxRecNum, filter)
    , cartManager_(cartManager)
{
}

bool BOSRecommender::recommend(
    const std::vector<itemid_t>& inputItemVec,
    std::vector<RecommendItem>& recItemVec
)
{
    if (inputItemVec.empty() == false)
    {
        // use input items as shopping cart
        return recommendImpl_(inputItemVec, recItemVec);
    }

    std::vector<itemid_t> cartItemVec;
    if (getCartItems_(cartItemVec))
    {
        return recommendImpl_(cartItemVec, recItemVec);
    }

    return false;
}

bool BOSRecommender::recommendImpl_(
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
    setReasonEvent_(recItemVec, RecommendSchema::CART_EVENT);

    return true;
}

bool BOSRecommender::getCartItems_(std::vector<itemid_t>& cartItemVec) const
{
    if (userId_ == 0)
    {
        LOG(ERROR) << "failed to recommend with empty shopping cart and empty user id";
        return false;
    }

    if (cartManager_->getCart(userId_, cartItemVec) == false)
    {
        LOG(ERROR) << "failed to get shopping cart items for user id " << userId_;
        return false;
    }

    return true;
}

} // namespace sf1r
