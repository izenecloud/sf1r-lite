#include "UserEventFilter.h"
#include "ItemFilter.h"
#include "PurchaseManager.h"
#include "CartManager.h"
#include "EventManager.h"
#include <bundles/recommend/RecommendSchema.h>

#include <glog/logging.h>

namespace
{

using namespace sf1r;

void excludeItems(
    const ItemIdSet& excludeItemSet,
    std::vector<itemid_t>& inputItems
)
{
    if (excludeItemSet.empty())
        return;

    std::vector<itemid_t> newInputItems;
    for (std::vector<itemid_t>::const_iterator it = inputItems.begin(); 
            it != inputItems.end(); ++it)
    {
        if (excludeItemSet.find(*it) == excludeItemSet.end())
        {
            newInputItems.push_back(*it);
        }
    }

    newInputItems.swap(inputItems);
}

}

namespace sf1r
{

UserEventFilter::UserEventFilter(
    PurchaseManager& purchaseManager,
    CartManager& cartManager,
    EventManager& eventManager
)
    : purchaseManager_(purchaseManager)
    , cartManager_(cartManager)
    , eventManager_(eventManager)
{
}

bool UserEventFilter::filter(
    userid_t userId,
    std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter
) const
{
    assert(userId);

    return (filterPurchaseItem_(userId, filter)
            && filterCartItem_(userId, filter)
            && filterPreferenceItem_(userId, inputItemVec, filter));
}

bool UserEventFilter::filterPurchaseItem_(
    userid_t userId,
    ItemFilter& filter
) const
{
    ItemIdSet purchaseItemSet;
    if (! purchaseManager_.getPurchaseItemSet(userId, purchaseItemSet))
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
    if (! cartManager_.getCart(userId, cartItemVec))
    {
        LOG(ERROR) << "failed to get shopping cart items for user id " << userId;
        return false;
    }

    filter.insert(cartItemVec.begin(), cartItemVec.end());
    return true;
}

bool UserEventFilter::filterPreferenceItem_(
    userid_t userId,
    std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter
) const
{
    EventManager::EventItemMap eventItemMap;
    if (! eventManager_.getEvent(userId, eventItemMap))
    {
        LOG(ERROR) << "failed to get events for user id " << userId;
        return false;
    }

    for (EventManager::EventItemMap::const_iterator it = eventItemMap.begin();
        it != eventItemMap.end(); ++it)
    {
        filter.insert(it->second.begin(), it->second.end());
    }

    const ItemIdSet& notRecInputSet = eventItemMap[RecommendSchema::NOT_REC_INPUT_EVENT];
    excludeItems(notRecInputSet, inputItemVec);

    return true;
}

bool UserEventFilter::addUserEvent(
    userid_t userId,
    std::vector<itemid_t>& inputItemVec,
    ItemEventMap& itemEventMap,
    ItemFilter& filter
) const
{
    assert(userId);

    return (addPurchaseItem_(userId, inputItemVec, itemEventMap)
            && addCartItem_(userId, inputItemVec, itemEventMap)
            && addPreferenceItem_(userId, inputItemVec, itemEventMap, filter));
}

bool UserEventFilter::addPurchaseItem_(
    userid_t userId,
    std::vector<itemid_t>& inputItemVec,
    ItemEventMap& itemEventMap
) const
{
    ItemIdSet purchaseItemSet;
    if (! purchaseManager_.getPurchaseItemSet(userId, purchaseItemSet))
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
    std::vector<itemid_t>& inputItemVec,
    ItemEventMap& itemEventMap
) const
{
    std::vector<itemid_t> cartItemVec;
    if (! cartManager_.getCart(userId, cartItemVec))
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
    std::vector<itemid_t>& inputItemVec,
    ItemEventMap& itemEventMap,
    ItemFilter& filter
) const
{
    EventManager::EventItemMap eventItemMap;
    if (! eventManager_.getEvent(userId, eventItemMap))
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

    const ItemIdSet& notRecInputSet = eventItemMap[RecommendSchema::NOT_REC_INPUT_EVENT];
    excludeItems(notRecInputSet, inputItemVec);

    return true;
}

} // namespace sf1r
