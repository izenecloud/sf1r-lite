#include "RecommendSearchService.h"
#include <recommend-manager/User.h>
#include <recommend-manager/UserManager.h>

namespace sf1r
{

RecommendSearchService::RecommendSearchService(UserManager* userManager)
    :userManager_(userManager)
{
}

bool RecommendSearchService::getUser(const std::string& userIdStr, User& user)
{
    // TODO get user.id_ from IDManager
    userid_t userId = 0;
    return userManager_->getUser(userId, user);
}

} // namespace sf1r
