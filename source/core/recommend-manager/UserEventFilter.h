/**
 * @file UserEventFilter.h
 * @brief filter user event items
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef USER_EVENT_FILTER_H
#define USER_EVENT_FILTER_H

#include "RecTypes.h"

#include <vector>
#include <map>

namespace sf1r
{
class ItemFilter;
class PurchaseManager;
class CartManager;
class EventManager;
class RateManager;

class UserEventFilter
{
public:
    UserEventFilter(
        PurchaseManager& purchaseManager,
        CartManager& cartManager,
        EventManager& eventManager,
        RateManager& rateManager
    );

    bool filter(
        userid_t userId,
        std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter
    ) const;

    typedef std::map<itemid_t, std::string> ItemEventMap;
    bool addUserEvent(
        userid_t userId,
        std::vector<itemid_t>& inputItemVec,
        ItemRateMap& itemRateMap,
        ItemEventMap& itemEventMap,
        ItemFilter& filter
    ) const;

private:
    bool filterRateItem_(
        userid_t userId,
        ItemFilter& filter
    ) const;

    bool filterPurchaseItem_(
        userid_t userId,
        ItemFilter& filter
    ) const;

    bool filterCartItem_(
        userid_t userId,
        ItemFilter& filter
    ) const;

    bool filterPreferenceItem_(
        userid_t userId,
        std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter
    ) const;

    bool addRateItem_(
        userid_t userId,
        ItemRateMap& itemRateMap,
        ItemEventMap& itemEventMap
    ) const;

    bool addPurchaseItem_(
        userid_t userId,
        std::vector<itemid_t>& inputItemVec,
        ItemEventMap& itemEventMap
    ) const;

    bool addCartItem_(
        userid_t userId,
        std::vector<itemid_t>& inputItemVec,
        ItemEventMap& itemEventMap
    ) const;

    bool addPreferenceItem_(
        userid_t userId,
        std::vector<itemid_t>& inputItemVec,
        ItemRateMap& itemRateMap,
        ItemEventMap& itemEventMap,
        ItemFilter& filter
    ) const;

protected:
    PurchaseManager& purchaseManager_;
    CartManager& cartManager_;
    EventManager& eventManager_;
    RateManager& rateManager_;
};

} // namespace sf1r

#endif // USER_EVENT_FILTER_H
