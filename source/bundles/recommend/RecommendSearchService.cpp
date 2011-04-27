#include "RecommendSearchService.h"
#include <recommend-manager/User.h>
#include <recommend-manager/Item.h>
#include <recommend-manager/UserManager.h>
#include <recommend-manager/ItemManager.h>

#include <glog/logging.h>

namespace sf1r
{

RecommendSearchService::RecommendSearchService(
    UserManager* userManager,
    ItemManager* itemManager,
    RecIdGenerator* userIdGenerator,
    RecIdGenerator* itemIdGenerator
)
    :userManager_(userManager)
    ,itemManager_(itemManager)
    ,userIdGenerator_(userIdGenerator)
    ,itemIdGenerator_(itemIdGenerator)
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

bool RecommendSearchService::getItem(const std::string& itemIdStr, Item& item)
{
    itemid_t itemId = 0;

    if (itemIdGenerator_->conv(itemIdStr, itemId, false) == false)
    {
        LOG(ERROR) << "error in getItem(), item id " << itemIdStr << " not yet added before";
        return false;
    }

    return itemManager_->getItem(itemId, item);
}

} // namespace sf1r
