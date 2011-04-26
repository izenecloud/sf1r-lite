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

namespace sf1r
{
class User;
class Item;
class UserManager;
class ItemManager;
class VisitManager;
class RecommendBundleConfiguration;

class RecommendTaskService : public ::izenelib::osgi::IService
{
public:
    RecommendTaskService(
        RecommendBundleConfiguration* bundleConfig,
        UserManager* userManager,
        ItemManager* itemManager,
        VisitManager* visitManager,
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

private:
    RecommendBundleConfiguration* bundleConfig_;
    UserManager* userManager_;
    ItemManager* itemManager_;
    VisitManager* visitManager_;
    RecIdGenerator* userIdGenerator_;
    RecIdGenerator* itemIdGenerator_;
};

} // namespace sf1r

#endif // RECOMMEND_TASK_SERVICE_H
