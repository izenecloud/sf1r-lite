/**
 * @file RecommendTaskService.h
 * @author Jun Jiang
 * @date 2011-04-20
 */

#ifndef RECOMMEND_TASK_SERVICE_H
#define RECOMMEND_TASK_SERVICE_H

#include <recommend-manager/RecTypes.h>

#include <util/osgi/IService.h>

#include <string>
#include <vector>

namespace sf1r
{
class User;
class Item;
class UserManager;
class ItemManager;
class VisitManager;
class PurchaseManager;
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
     * @p userIdStr and @p itemIdStr must not be empty.
     */
    bool visitItem(const std::string& userIdStr, const std::string& itemIdStr);

    struct OrderItem
    {
        OrderItem()
            :quantity_(0)
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

private:
    /**
     * Load user SCD files.
     */
    bool loadUserSCD_();

    /**
     * Load item SCD files.
     */
    bool loadItemSCD_();

    /**
     * Load order SCD files.
     */
    bool loadOrderSCD_();

    /** user id, order id */
    typedef std::pair<string, string> OrderKey;
    typedef std::map<OrderKey, OrderItemVec> OrderMap;

    /**
     * Load the orders in @p orderMap.
     * @param orderMap the orders
     */
    void loadOrderMap_(const OrderMap& orderMap);

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

    bool loadOrder_(
        const std::string& userIdStr,
        const OrderItemVec& orderItemVec
    );

private:
    RecommendBundleConfiguration* bundleConfig_;
    directory::DirectoryRotator* directoryRotator_;
    UserManager* userManager_;
    ItemManager* itemManager_;
    VisitManager* visitManager_;
    PurchaseManager* purchaseManager_;
    OrderManager* orderManager_;
    RecIdGenerator* userIdGenerator_;
    RecIdGenerator* itemIdGenerator_;
    JobScheduler* jobScheduler_;
};

} // namespace sf1r

#endif // RECOMMEND_TASK_SERVICE_H
