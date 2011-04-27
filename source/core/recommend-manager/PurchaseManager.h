/**
 * @file PurchaseManager.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef PURCHASE_MANAGER_H
#define PURCHASE_MANAGER_H

#include "RecTypes.h"
#include "SDBCursorIterator.h"
#include <sdb/SequentialDB.h>

#include <string>
#include <vector>

#include <boost/serialization/set.hpp> // serialize ItemIdSet

namespace sf1r
{

class PurchaseManager
{
public:
    PurchaseManager(const std::string& path);

    void flush();

    struct OrderItem
    {
        OrderItem(itemid_t itemId, int quantity, double price)
            :itemId_(itemId)
            ,quantity_(quantity)
            ,price_(price)
        {}

        itemid_t itemId_;
        int quantity_;
        double price_;
    };
    typedef std::vector<OrderItem> OrderItemVec;

    /**
     * Add a purchase event.
     * @param userId the user id
     * @param orderItemVec the items in the order
     * @param orderIdStr the order id
     * @return true for succcess, false for failure
     */
    bool addPurchaseItem(
        userid_t userId,
        const OrderItemVec& orderItemVec,
        const std::string& orderIdStr
    );

    bool getPurchaseItemSet(userid_t userId, ItemIdSet& itemIdSet);

    /**
     * The number of users who have purchased items.
     */
    unsigned int purchaseUserNum();

    typedef izenelib::sdb::unordered_sdb_tc<userid_t, ItemIdSet, ReadWriteLock> SDBType;
    typedef SDBCursorIterator<SDBType> SDBIterator;
    SDBIterator begin();
    SDBIterator end();

private:
    SDBType container_;
};

} // namespace sf1r

#endif // PURCHASE_MANAGER_H
