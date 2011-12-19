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
#include <iostream>

#include <boost/serialization/set.hpp> // serialize ItemIdSet

namespace sf1r
{

class PurchaseManager
{
public:
    PurchaseManager(
        const std::string& path,
        ItemCFManager& itemCFManager
    );

    ~PurchaseManager();

    void flush();

    /**
     * Add a purchase event.
     * @param userId the user id
     * @param itemVec the items in the order
     * @param isUpdateSimMatrix true for call @p ItemCFManager::updateMatrix() to also update similarity matrix,
     *                          false for call @p ItemCFManager::updateVisitMatrix() to only update visit matrix.
     * @return true for succcess, false for failure
     */
    bool addPurchaseItem(
        userid_t userId,
        const std::vector<itemid_t>& itemVec,
        bool isUpdateSimMatrix = true
    );

    /**
     * Rebuild the whole similarity matrix in batch mode.
     */
    void buildSimMatrix();

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

    void print(std::ostream& ostream) const;

private:
    SDBType container_;
    ItemCFManager& itemCFManager_;
};

std::ostream& operator<<(std::ostream& out, const PurchaseManager& purchaseManager);

} // namespace sf1r

#endif // PURCHASE_MANAGER_H
