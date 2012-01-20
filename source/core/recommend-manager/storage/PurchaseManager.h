/**
 * @file PurchaseManager.h
 * @author Jun Jiang
 * @date 2012-01-20
 */

#ifndef PURCHASE_MANAGER_H
#define PURCHASE_MANAGER_H

#include <recommend-manager/RecTypes.h>

#include <string>
#include <vector>
#include <list>

namespace sf1r
{
class RecommendMatrix;

class PurchaseManager
{
public:
    virtual ~PurchaseManager() {}

    virtual void flush() {}

    /**
     * Add a purchase event.
     * @param userId the user id
     * @param itemVec the items in the order
     * @param matrix if not NULL, @c RecommendMatrix::update() would be called
     * @return true for succcess, false for failure
     */
    bool addPurchaseItem(
        const std::string& userId,
        const std::vector<itemid_t>& itemVec,
        RecommendMatrix* matrix
    );

    /**
     * Get @p itemIdSet purchased by @p userId.
     * @param userId user id
     * @param itemidSet item id set purchased by @p userId, it would be empty
     *                  if @p userId has not purchased any item.
     * @return true for success, false for error happened.
     */
    virtual bool getPurchaseItemSet(
        const std::string& userId,
        ItemIdSet& itemIdSet
    ) = 0;

protected:
    /**
     * Save the purchase items by @p userId.
     * @param userId user id
     * @param totalItems all items purchased by @p userId, including @p newItems
     * @param newItems the new items not purchased by @p userId before
     * @return true for success, false for error happened.
     * @note the implementation could choose to save either @p totalItems
     *       or @p newItems according to its storage policy
     */
    virtual bool savePurchaseItem_(
        const std::string& userId,
        const ItemIdSet& totalItems,
        const std::list<sf1r::itemid_t>& newItems
    ) = 0;
};

} // namespace sf1r

#endif // PURCHASE_MANAGER_H
