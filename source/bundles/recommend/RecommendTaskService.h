/**
 * @file RecommendTaskService.h
 * @author Jun Jiang
 * @date 2011-04-20
 */

#ifndef RECOMMEND_TASK_SERVICE_H
#define RECOMMEND_TASK_SERVICE_H

#include <recommend-manager/RecTypes.h>

#include <util/osgi/IService.h>
#include <util/cronexpression.h>

#include <string>
#include <vector>

#include <boost/thread/mutex.hpp>

namespace sf1r
{
class User;
class Item;
class UserManager;
class ItemManager;
class VisitManager;
class PurchaseManager;
class CartManager;
class EventManager;
class OrderManager;
class RecommendBundleConfiguration;
class JobScheduler;

namespace directory
{
class DirectoryRotator;
}

class RecommendTaskService : public ::izenelib::osgi::IService
{
public:
    RecommendTaskService(
        RecommendBundleConfiguration* bundleConfig,
        directory::DirectoryRotator* directoryRotator,
        UserManager* userManager,
        ItemManager* itemManager,
        VisitManager* visitManager,
        PurchaseManager* purchaseManager,
        CartManager* cartManager,
        EventManager* eventManager,
        OrderManager* orderManager,
        RecIdGenerator* userIdGenerator,
        RecIdGenerator* itemIdGenerator
    );

    ~RecommendTaskService();

    /**
     * Build collection from 3 types of SCD files: user, item, purchase record.
     */
    bool buildCollection();

    /**
     * @p user.idStr_ must not be empty.
     */
    bool addUser(const User& user);

    /**
     * @p user.idStr_ must not be empty.
     */
    bool updateUser(const User& user);

    /**
     * @p userIdStr must not be empty.
     */
    bool removeUser(const std::string& userIdStr);

    /**
     * @p item.idStr_ must not be empty.
     */
    bool addItem(const Item& item);

    /**
     * @p item.idStr_ must not be empty.
     */
    bool updateItem(const Item& item);

    /**
     * @p itemIdStr must not be empty.
     */
    bool removeItem(const std::string& itemIdStr);

    /**
     * @p sessionIdStr, @p userIdStr and @p itemIdStr must not be empty.
     * @param isRecItem whether is recommendation item.
     */
    bool visitItem(
        const std::string& sessionIdStr,
        const std::string& userIdStr,
        const std::string& itemIdStr,
        bool isRecItem
    );

    struct OrderItem
    {
        OrderItem()
            :quantity_(0)
            ,price_(0.0)
        {}

        OrderItem(const std::string& itemIdStr)
            :itemIdStr_(itemIdStr)
            ,quantity_(0)
            ,price_(0.0)
        {}

        OrderItem(const std::string& itemIdStr, int quantity, double price)
            :itemIdStr_(itemIdStr)
            ,quantity_(quantity)
            ,price_(price)
        {}

        std::string itemIdStr_;
        int quantity_;
        double price_;
        std::string dateStr_;
    };
    typedef std::vector<OrderItem> OrderItemVec;

    /**
     * Add a purchase event.
     * @param userIdStr the user id, it must not be empty.
     * @param orderIdStr the order id
     * @param orderItemVec the items in the order, each item's id must not be empty
     * @return true for succcess, false for failure
     */
    bool purchaseItem(
        const std::string& userIdStr,
        const std::string& orderIdStr,
        const OrderItemVec& orderItemVec
    );

    /**
     * Update shopping cart event.
     * @param userIdStr the user id, it must not be empty.
     * @param cartItemVec the items in the user's shopping cart, each item id must not be empty,
     *                    an empty @p cartItemVec means the shopping cart is empty.
     * @return true for succcess, false for failure
     */
    bool updateShoppingCart(
        const std::string& userIdStr,
        const OrderItemVec& cartItemVec
    );

    /**
     * Track a user event.
     * @param isAdd true for add this event, false for remove this event
     * @param eventStr the event type
     * @param userIdStr the user id
     * @param itemIdStr the item id
     * @return true for succcess, false for failure
     */
    bool trackEvent(
        bool isAdd,
        const std::string& eventStr,
        const std::string& userIdStr,
        const std::string& itemIdStr
    );

private:
    /**
     * Convert from @p userIdStr to @p userId,
     * and from @p orderItemVec to @p itemIdVec.
     * @return true for success, false for failure.
     */
    bool convertUserItemId_(
        const std::string& userIdStr,
        const OrderItemVec& orderItemVec,
        userid_t& userId,
        std::vector<itemid_t>& itemIdVec
    );

    /**
     * Load user SCD files.
     */
    bool loadUserSCD_();

    /**
     * Parse user SCD file.
     * @param scdPath the user SCD file path
     * @return true for success, false for failure
     */
    bool parseUserSCD_(const std::string& scdPath);

    /**
     * Load item SCD files.
     */
    bool loadItemSCD_();

    /**
     * Parse item SCD file.
     * @param scdPath the item SCD file path
     * @return true for success, false for failure
     */
    bool parseItemSCD_(const std::string& scdPath);

    /**
     * Load order SCD files.
     */
    bool loadOrderSCD_();

    /**
     * Parse order SCD file.
     * @param scdPath the order SCD file path
     * @return true for success, false for failure
     */
    bool parseOrderSCD_(const std::string& scdPath);

    /** user id, order id */
    typedef std::pair<string, string> OrderKey;
    typedef std::map<OrderKey, OrderItemVec> OrderMap;

    /**
     * load order item.
     * if @p orderIdStr is empty, it stores @p orderItem directly,
     * otherwise, it inserts @p orderItem into @p orderMap.
     */
    void loadOrderItem_(
        const std::string& userIdStr,
        const std::string& orderIdStr,
        const OrderItem& orderItem,
        OrderMap& orderMap
    );

    /**
     * it calls @c saveOrder_() for each order in @p orderMap.
     * @param orderMap the orders
     */
    void saveOrderMap_(const OrderMap& orderMap);

    /**
     * save the order into @c orderManager_, @c purchaseManager_.
     * @param userIdStr the string of user id
     * @param orderIdStr the string of order id
     * @param orderItemVec the array of item id string
     * @param isUpdateSimMatrix this param is passed to @c PurchaseManager::addPurchaseItem().
     */
    bool saveOrder_(
        const std::string& userIdStr,
        const std::string& orderIdStr,
        const OrderItemVec& orderItemVec,
        bool isUpdateSimMatrix
    );

    /**
     * insert the order items as DB records.
     */
    bool insertOrderDB_(
        const std::string& userIdStr,
        const std::string& orderIdStr,
        const OrderItemVec& orderItemVec,
        userid_t userId,
        const std::vector<itemid_t>& itemIdVec
    );

    void buildFreqItemSet_();
    void cronJob_();

private:
    RecommendBundleConfiguration* bundleConfig_;
    directory::DirectoryRotator* directoryRotator_;
    UserManager* userManager_;
    ItemManager* itemManager_;
    VisitManager* visitManager_;
    PurchaseManager* purchaseManager_;
    CartManager* cartManager_;
    EventManager* eventManager_;
    OrderManager* orderManager_;
    RecIdGenerator* userIdGenerator_;
    RecIdGenerator* itemIdGenerator_;
    JobScheduler* jobScheduler_;
    boost::mutex freqItemMutex_; // for build freq item set
    izenelib::util::CronExpression cronExpression_;
    const std::string cronJobName_;
};

} // namespace sf1r

#endif // RECOMMEND_TASK_SERVICE_H
