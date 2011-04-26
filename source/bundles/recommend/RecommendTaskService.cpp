#include "RecommendTaskService.h"
#include "RecommendBundleConfiguration.h"
#include <recommend-manager/User.h>
#include <recommend-manager/UserManager.h>

#include <glog/logging.h>

namespace sf1r
{

RecommendTaskService::RecommendTaskService(
    RecommendBundleConfiguration* bundleConfig,
    UserManager* userManager,
    RecIdGenerator* userIdGenerator
)
    :bundleConfig_(bundleConfig)
    ,userManager_(userManager)
    ,userIdGenerator_(userIdGenerator)
{
}

bool RecommendTaskService::addUser(const User& user)
{
    if (user.idStr_.empty())
    {
        return false;
    }

    userid_t userId = 0;

    if (userIdGenerator_->conv(user.idStr_, userId))
    {
        LOG(WARNING) << "in addUser(), user id " << user.idStr_ << " already exists";
    }

    return userManager_->addUser(userId, user);
}

bool RecommendTaskService::updateUser(const User& user)
{
    if (user.idStr_.empty())
    {
        return false;
    }

    userid_t userId = 0;

    if (userIdGenerator_->conv(user.idStr_, userId, false) == false)
    {
        LOG(ERROR) << "error in updateUser(), user id " << user.idStr_ << " not yet added before";
        return false;
    }

    return userManager_->updateUser(userId, user);
}

bool RecommendTaskService::removeUser(const std::string& userIdStr)
{
    if (userIdStr.empty())
    {
        return false;
    }

    userid_t userId = 0;

    if (userIdGenerator_->conv(userIdStr, userId, false) == false)
    {
        LOG(ERROR) << "error in removeUser(), user id " << userIdStr << " not yet added before";
        return false;
    }

    return userManager_->removeUser(userId);
}

} // namespace sf1r
