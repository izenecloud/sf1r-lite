#include "RecommendSearchService.h"
#include <recommend-manager/User.h>
#include <recommend-manager/UserManager.h>

#include <glog/logging.h>

namespace sf1r
{

RecommendSearchService::RecommendSearchService(
    UserManager* userManager,
    RecIdGenerator* userIdGenerator
)
    :userManager_(userManager)
    ,userIdGenerator_(userIdGenerator)
{
}

bool RecommendSearchService::getUser(const std::string& userIdStr, User& user)
{
    userid_t userId = 0;

    if (userIdGenerator_->conv(userIdStr, userId, false) == false)
    {
        LOG(ERROR) << "error in getUser(), user id " << userIdStr << " not yet added before";
        return false;
    }

    return userManager_->getUser(userId, user);
}

} // namespace sf1r
