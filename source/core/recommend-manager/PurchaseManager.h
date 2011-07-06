/**
 * @file PurchaseManager.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef PURCHASE_MANAGER_H
#define PURCHASE_MANAGER_H

#include "RecTypes.h"
#include <sdb/SequentialDB.h>
#include <sdb/SDBCursorIterator.h>

#include <string>
#include <vector>

#include <boost/serialization/set.hpp> // serialize ItemIdSet

namespace sf1r
{
class ItemManager;

class PurchaseManager
{
public:
    PurchaseManager(
        const std::string& path,
        ItemCFManager* itemCFManager,
        const ItemManager* itemManager
    );

    ~PurchaseManager();

    void flush();

    /**
     * Add a purchase event.
     * @param userId the user id
     * @param itemVec the items in the order
     * @return true for succcess, false for failure
     */
    bool addPurchaseItem(
        userid_t userId,
        const std::vector<itemid_t>& itemVec,
        bool isBuildUserResult = true
    );

    /**
     * Build recommend items for user.
     * @param userId the user id
     */
    void buildUserResult(userid_t userId);

    /**
     * Get @p itemIdSet purchased by @p userId.
     * @param userId user id
     * @param itemidSet item id set purchased by @p userId, it would be empty if @p userId
     *                  has not purchased any item.
     * @return true for success, false for error happened.
     */
    bool getPurchaseItemSet(userid_t userId, ItemIdSet& itemIdSet);

    /**
     * The number of users who have purchased items.
     */
    unsigned int purchaseUserNum();

    typedef izenelib::sdb::unordered_sdb_tc<userid_t, ItemIdSet, ReadWriteLock> SDBType;
    typedef izenelib::sdb::SDBCursorIterator<SDBType> SDBIterator;
    SDBIterator begin();
    SDBIterator end();

private:
    SDBType container_;
    ItemCFManager* itemCFManager_;
    const ItemManager* itemManager_;
};

} // namespace sf1r

#endif // PURCHASE_MANAGER_H
