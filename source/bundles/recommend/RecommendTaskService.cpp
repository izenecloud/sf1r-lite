#include "RecommendTaskService.h"
#include "RecommendBundleConfiguration.h"
#include <recommend-manager/User.h>
#include <recommend-manager/UserManager.h>

namespace sf1r
{

RecommendTaskService::RecommendTaskService(
    RecommendBundleConfiguration* bundleConfig,
    UserManager* userManager
)
    :bundleConfig_(bundleConfig)
    ,userManager_(userManager)
{
}

bool RecommendTaskService::addUser(const User& user)
{
    if (user.idStr_.empty())
    {
        return false;
    }

    // TODO get user.id_ from IDManager
    return userManager_->addUser(user);
}

} // namespace sf1r
