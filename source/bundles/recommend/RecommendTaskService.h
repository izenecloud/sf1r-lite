/**
 * @file RecommendTaskService.h
 * @author Jun Jiang
 * @date 2011-04-20
 */

#ifndef RECOMMEND_TASK_SERVICE_H
#define RECOMMEND_TASK_SERVICE_H

#include <util/osgi/IService.h>

#include <string>

namespace sf1r
{
class User;
class UserManager;
class RecommendBundleConfiguration;

class RecommendTaskService : public ::izenelib::osgi::IService
{
public:
    RecommendTaskService(
        RecommendBundleConfiguration* bundleConfig,
        UserManager* userManager
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

private:
    RecommendBundleConfiguration* bundleConfig_;
    UserManager* userManager_;
};

} // namespace sf1r

#endif // RECOMMEND_TASK_SERVICE_H
