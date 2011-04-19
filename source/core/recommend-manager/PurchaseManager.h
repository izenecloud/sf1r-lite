/**
 * @file PurchaseManager.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef PURCHASE_MANAGER_H
#define PURCHASE_MANAGER_H

#include "RecTypes.h"
#include "Purchase.h"
#include "SDBCursorIterator.h"
#include <sdb/SequentialDB.h>

#include <string>

namespace sf1r
{

class PurchaseManager
{
public:
    PurchaseManager(const std::string& path);

    void flush();

    /**
     * Add a purchase record.
     * @param orderIdStr the order id,
     * if user purchases multiple items in one transaction,
     * these items should be added with the same order id,
     * if @p orderIdStr is empty, it would add a new transaction containing only one item.
     */
    bool addPurchaseItem(
        userid_t userId,
        itemid_t itemId,
        double price,
        int quantity,
        const std::string& orderIdStr
    );

    bool getPurchaseItemSet(userid_t userId, ItemIdSet& itemIdSet);
    bool getPurchaseOrderVec(userid_t userId, OrderVec& orderVec);

    /**
     * The number of users who have purchased items.
     */
    unsigned int purchaseUserNum();

    typedef izenelib::sdb::unordered_sdb_tc<userid_t, Purchase, ReadWriteLock> SDBType;
    typedef SDBCursorIterator<SDBType> SDBIterator;
    SDBIterator begin();
    SDBIterator end();

private:
    SDBType container_;
};

} // namespace sf1r

#endif // PURCHASE_MANAGER_H
