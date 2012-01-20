#include "UserEventFilter.h"
#include "ItemFilter.h"
#include "storage/PurchaseManager.h"
#include "CartManager.h"
#include "EventManager.h"
#include "RateManager.h"
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

void excludeItems(
    const ItemIdSet& excludeItemSet,
    ItemRateMap& itemRateMap
)
{
    if (excludeItemSet.empty())
        return;

    for (ItemRateMap::iterator it = itemRateMap.begin();
        it != itemRateMap.end();)
    {
        if (excludeItemSet.find(it->first) == excludeItemSet.end())
        {
            ++it;
        }
        else
        {
            itemRateMap.erase(it++);
        }
    }
}

}

namespace sf1r
{

UserEventFilter::UserEventFilter(
    PurchaseManager& purchaseManager,
    CartManager& cartManager,
    EventManager& eventManager,
    RateManager& rateManager
)
    : purchaseManager_(purchaseManager)
    , cartManager_(cartManager)
    , eventManager_(eventManager)
    , rateManager_(rateManager)
{
}

bool UserEventFilter::filter(
    const std::string& userId,
    std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter
) const
{
    assert(!userId.empty());

    return (filterRateItem_(userId, filter)
            && filterPurchaseItem_(userId, filter)
            && filterCartItem_(userId, filter)
            && filterPreferenceItem_(userId, inputItemVec, filter));
}

bool UserEventFilter::filterRateItem_(
    const std::string& userId,
    ItemFilter& filter
) const
{
    ItemRateMap itemRateMap;
    if (! rateManager_.getItemRateMap(userId, itemRateMap))
    {
        LOG(ERROR) << "failed to get rates for user id " << userId;
        return false;
    }

    for (ItemRateMap::const_iterator it = itemRateMap.begin();
        it != itemRateMap.end(); ++it)
    {
        filter.insert(it->first);
    }

    return true;
}

bool UserEventFilter::filterPurchaseItem_(
    const std::string& userId,
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
    const std::string& userId,
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
    const std::string& userId,
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
    const std::string& userId,
    std::vector<itemid_t>& inputItemVec,
    ItemRateMap& itemRateMap,
    ItemEventMap& itemEventMap,
    ItemFilter& filter
) const
{
    assert(!userId.empty());

    inputItemVec.clear();

    return (addRateItem_(userId, itemRateMap, itemEventMap)
            && addPurchaseItem_(userId, inputItemVec, itemEventMap)
            && addCartItem_(userId, inputItemVec, itemEventMap)
            && addPreferenceItem_(userId, inputItemVec, itemRateMap, itemEventMap, filter));
}

bool UserEventFilter::addRateItem_(
    const std::string& userId,
    ItemRateMap& itemRateMap,
    ItemEventMap& itemEventMap
) const
{
    if (! rateManager_.getItemRateMap(userId, itemRateMap))
    {
        LOG(ERROR) << "failed to get rates for user id " << userId;
        return false;
    }

    for (ItemRateMap::iterator it = itemRateMap.begin();
        it != itemRateMap.end();)
    {
        ItemEventMap::value_type itemEvent(it->first, RecommendSchema::RATE_EVENT);
        if (itemEventMap.insert(itemEvent).second)
        {
            ++it;
        }
        else
        {
            itemRateMap.erase(it++);
        }
    }

    return true;
}

bool UserEventFilter::addPurchaseItem_(
    const std::string& userId,
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
    const std::string& userId,
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
    const std::string& userId,
    std::vector<itemid_t>& inputItemVec,
    ItemRateMap& itemRateMap,
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
    excludeItems(notRecInputSet, itemRateMap);

    return true;
}

} // namespace sf1r
