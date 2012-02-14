/**
 * @file UserEventFilter.h
 * @brief filter user event items
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef USER_EVENT_FILTER_H
#define USER_EVENT_FILTER_H

#include "../common/RecTypes.h"

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
        const std::string& userId,
        std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter
    ) const;

    typedef std::map<itemid_t, std::string> ItemEventMap;
    bool addUserEvent(
        const std::string& userId,
        std::vector<itemid_t>& inputItemVec,
        ItemRateMap& itemRateMap,
        ItemEventMap& itemEventMap,
        ItemFilter& filter
    ) const;

private:
    bool filterRateItem_(
        const std::string& userId,
        ItemFilter& filter
    ) const;

    bool filterPurchaseItem_(
        const std::string& userId,
        ItemFilter& filter
    ) const;

    bool filterCartItem_(
        const std::string& userId,
        ItemFilter& filter
    ) const;

    bool filterPreferenceItem_(
        const std::string& userId,
        std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter
    ) const;

    bool addRateItem_(
        const std::string& userId,
        ItemRateMap& itemRateMap,
        ItemEventMap& itemEventMap
    ) const;

    bool addPurchaseItem_(
        const std::string& userId,
        std::vector<itemid_t>& inputItemVec,
        ItemEventMap& itemEventMap
    ) const;

    bool addCartItem_(
        const std::string& userId,
        std::vector<itemid_t>& inputItemVec,
        ItemEventMap& itemEventMap
    ) const;

    bool addPreferenceItem_(
        const std::string& userId,
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
