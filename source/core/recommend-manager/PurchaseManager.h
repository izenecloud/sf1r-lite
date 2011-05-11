/**
 * @file PurchaseManager.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef PURCHASE_MANAGER_H
#define PURCHASE_MANAGER_H

#include "RecTypes.h"
#include "OrderManager.h"
#include <sdb/SequentialDB.h>
#include <sdb/SDBCursorIterator.h>

#include <string>
#include <vector>

#include <boost/serialization/set.hpp> // serialize ItemIdSet
#include <boost/thread/mutex.hpp>

namespace sf1r
{
class ItemManager;
class JobScheduler;

class PurchaseManager
{
public:
    PurchaseManager(
        const std::string& path,
        JobScheduler* jobScheduler,
        ItemCFManager* itemCFManager,
        const ItemManager* itemManager
    );

    ~PurchaseManager();

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

    OrderManager* getOrderManager() {
        return &orderManager_;
    }

private:
    orderid_t newOrderId();

private:
    SDBType container_;
    std::string orderManagerPath_;
    OrderManager orderManager_;
    JobScheduler* jobScheduler_;
    ItemCFManager* itemCFManager_;
    const ItemManager* itemManager_;
    std::string orderIdPath_;
    orderid_t orderId_;
    boost::mutex orderIdMutex_;
};

} // namespace sf1r

#endif // PURCHASE_MANAGER_H
