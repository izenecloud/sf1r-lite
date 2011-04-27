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
class RecommendBundleConfiguration;

class RecommendTaskService : public ::izenelib::osgi::IService
{
public:
    RecommendTaskService(
        RecommendBundleConfiguration* bundleConfig,
        UserManager* userManager,
        ItemManager* itemManager,
        VisitManager* visitManager,
        PurchaseManager* purchaseManager,
        RecIdGenerator* userIdGenerator,
        RecIdGenerator* itemIdGenerator
    );

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
     * @param orderItemVec the items in the order, each item's id must not be empty
     * @param orderIdStr the order id
     * @return true for succcess, false for failure
     */
    bool purchaseItem(
        const std::string& userIdStr,
        const OrderItemVec& orderItemVec,
        const std::string& orderIdStr
    );

private:
    RecommendBundleConfiguration* bundleConfig_;
    UserManager* userManager_;
    ItemManager* itemManager_;
    VisitManager* visitManager_;
    PurchaseManager* purchaseManager_;
    RecIdGenerator* userIdGenerator_;
    RecIdGenerator* itemIdGenerator_;
};

} // namespace sf1r

#endif // RECOMMEND_TASK_SERVICE_H
