#include "UserEventFilter.h"
#include "ItemFilter.h"
#include "PurchaseManager.h"
#include "CartManager.h"
#include "EventManager.h"
#include <bundles/recommend/RecommendSchema.h>

#include <glog/logging.h>

namespace sf1r
{

UserEventFilter::UserEventFilter(
    const RecommendSchema& schema,
    PurchaseManager* purchaseManager,
    CartManager* cartManager,
    EventManager* eventManager
)
    : recommendSchema_(schema)
    , purchaseManager_(purchaseManager)
    , cartManager_(cartManager)
    , eventManager_(eventManager)
{
}

bool UserEventFilter::filter(
    userid_t userId,
    ItemFilter& filter,
    ItemIdSet& notRecInputSet
) const
{
    assert(userId);

    return (filterPurchaseItem_(userId, filter)
            && filterCartItem_(userId, filter)
            && filterPreferenceItem_(userId, filter, notRecInputSet));
}

bool UserEventFilter::filterPurchaseItem_(
    userid_t userId,
    ItemFilter& filter
) const
{
    ItemIdSet purchaseItemSet;
    if (purchaseManager_->getPurchaseItemSet(userId, purchaseItemSet) == false)
    {
        LOG(ERROR) << "failed to get purchased items for user id " << userId;
        return false;
    }

    filter.insert(purchaseItemSet.begin(), purchaseItemSet.end());
    return true;
}

bool UserEventFilter::filterCartItem_(
    userid_t userId,
    ItemFilter& filter
) const
{
    std::vector<itemid_t> cartItemVec;
    if (cartManager_->getCart(userId, cartItemVec) == false)
    {
        LOG(ERROR) << "failed to get shopping cart items for user id " << userId;
        return false;
    }

    filter.insert(cartItemVec.begin(), cartItemVec.end());
    return true;
}

bool UserEventFilter::filterPreferenceItem_(
    userid_t userId,
    ItemFilter& filter,
    ItemIdSet& notRecInputSet
) const
{
    EventManager::EventItemMap eventItemMap;
    if (eventManager_->getEvent(userId, eventItemMap) == false)
    {
        LOG(ERROR) << "failed to get events for user id " << userId;
        return false;
    }

    for (EventManager::EventItemMap::const_iterator it = eventItemMap.begin();
        it != eventItemMap.end(); ++it)
    {
        filter.insert(it->second.begin(), it->second.end());
    }

    notRecInputSet.swap(eventItemMap[RecommendSchema::NOT_REC_INPUT_EVENT]);

    return true;
}

bool UserEventFilter::addUserEvent(
    userid_t userId,
    ItemEventMap& itemEventMap,
    std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    ItemIdSet& notRecInputSet
) const
{
    assert(userId);

    return (addPurchaseItem_(userId, itemEventMap, inputItemVec)
            && addCartItem_(userId, itemEventMap, inputItemVec)
            && addPreferenceItem_(userId, itemEventMap, inputItemVec, filter, notRecInputSet));
}

bool UserEventFilter::addPurchaseItem_(
    userid_t userId,
    ItemEventMap& itemEventMap,
    std::vector<itemid_t>& inputItemVec
) const
{
    ItemIdSet purchaseItemSet;
    if (purchaseManager_->getPurchaseItemSet(userId, purchaseItemSet) == false)
    {
        LOG(ERROR) << "failed to get purchased items for user id " << userId;
        return false;
    }

    for (ItemIdSet::const_iterator it = purchaseItemSet.begin();
        it != purchaseItemSet.end(); ++it)
    {
        ItemEventMap::value_type itemEvent(*it, RecommendSchema::PURCHASE_EVENT);
        if (itemEventMap.insert(itemEvent).second)
        {
            inputItemVec.push_back(*it);
        }
    }

    return true;
}

bool UserEventFilter::addCartItem_(
    userid_t userId,
    ItemEventMap& itemEventMap,
    std::vector<itemid_t>& inputItemVec
) const
{
    std::vector<itemid_t> cartItemVec;
    if (cartManager_->getCart(userId, cartItemVec) == false)
    {
        LOG(ERROR) << "failed to get shopping cart items for user id " << userId;
        return false;
    }

    for (std::vector<itemid_t>::const_iterator it = cartItemVec.begin();
        it != cartItemVec.end(); ++it)
    {
        ItemEventMap::value_type itemEvent(*it, RecommendSchema::CART_EVENT);
        if (itemEventMap.insert(itemEvent).second)
        {
            inputItemVec.push_back(*it);
        }
    }

    return true;
}

bool UserEventFilter::addPreferenceItem_(
    userid_t userId,
    ItemEventMap& itemEventMap,
    std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    ItemIdSet& notRecInputSet
) const
{
    EventManager::EventItemMap eventItemMap;
    if (eventManager_->getEvent(userId, eventItemMap) == false)
    {
        LOG(ERROR) << "failed to get events for user id " << userId;
        return false;
    }

    for (EventManager::EventItemMap::const_iterator eventIt = eventItemMap.begin();
        eventIt != eventItemMap.end(); ++eventIt)
    {
        if (eventIt->first == RecommendSchema::NOT_REC_RESULT_EVENT
            || eventIt->first == RecommendSchema::NOT_REC_INPUT_EVENT)
        {
            filter.insert(eventIt->second.begin(), eventIt->second.end());
        }
        else
        {
            const std::string& eventName = eventIt->first;
            const ItemIdSet& eventItems = eventIt->second;
            for (ItemIdSet::const_iterator it = eventItems.begin();
                it != eventItems.end(); ++it)
            {
                ItemEventMap::value_type itemEvent(*it, eventName);
                if (itemEventMap.insert(itemEvent).second)
                {
                    inputItemVec.push_back(*it);
                }
            }
        }
    }

    notRecInputSet.swap(eventItemMap[RecommendSchema::NOT_REC_INPUT_EVENT]);

    return true;
}

} // namespace sf1r
