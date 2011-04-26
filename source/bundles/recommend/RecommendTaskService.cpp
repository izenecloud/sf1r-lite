#include "RecommendTaskService.h"
#include "RecommendBundleConfiguration.h"
#include <recommend-manager/User.h>
#include <recommend-manager/Item.h>
#include <recommend-manager/UserManager.h>
#include <recommend-manager/ItemManager.h>

#include <glog/logging.h>

namespace sf1r
{

RecommendTaskService::RecommendTaskService(
    RecommendBundleConfiguration* bundleConfig,
    UserManager* userManager,
    ItemManager* itemManager,
    RecIdGenerator* userIdGenerator,
    RecIdGenerator* itemIdGenerator
)
    :bundleConfig_(bundleConfig)
    ,userManager_(userManager)
    ,itemManager_(itemManager)
    ,userIdGenerator_(userIdGenerator)
    ,itemIdGenerator_(itemIdGenerator)
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

bool RecommendTaskService::addItem(const Item& item)
{
    if (item.idStr_.empty())
    {
        return false;
    }

    itemid_t itemId = 0;

    if (itemIdGenerator_->conv(item.idStr_, itemId))
    {
        LOG(WARNING) << "in addItem(), item id " << item.idStr_ << " already exists";
    }

    return itemManager_->addItem(itemId, item);
}

bool RecommendTaskService::updateItem(const Item& item)
{
    if (item.idStr_.empty())
    {
        return false;
    }

    itemid_t itemId = 0;

    if (itemIdGenerator_->conv(item.idStr_, itemId, false) == false)
    {
        LOG(ERROR) << "error in updateItem(), item id " << item.idStr_ << " not yet added before";
        return false;
    }

    return itemManager_->updateItem(itemId, item);
}

bool RecommendTaskService::removeItem(const std::string& itemIdStr)
{
    if (itemIdStr.empty())
    {
        return false;
    }

    itemid_t itemId = 0;

    if (itemIdGenerator_->conv(itemIdStr, itemId, false) == false)
    {
        LOG(ERROR) << "error in removeItem(), item id " << itemIdStr << " not yet added before";
        return false;
    }

    return itemManager_->removeItem(itemId);
}

} // namespace sf1r
