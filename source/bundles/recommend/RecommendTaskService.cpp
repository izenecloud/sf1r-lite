#include "RecommendTaskService.h"
#include "RecommendBundleConfiguration.h"
#include <recommend-manager/User.h>
#include <recommend-manager/Item.h>
#include <recommend-manager/UserManager.h>
#include <recommend-manager/ItemManager.h>
#include <recommend-manager/VisitManager.h>
#include <recommend-manager/PurchaseManager.h>

#include <glog/logging.h>

namespace sf1r
{

RecommendTaskService::RecommendTaskService(
    RecommendBundleConfiguration* bundleConfig,
    UserManager* userManager,
    ItemManager* itemManager,
    VisitManager* visitManager,
    PurchaseManager* purchaseManager,
    RecIdGenerator* userIdGenerator,
    RecIdGenerator* itemIdGenerator
)
    :bundleConfig_(bundleConfig)
    ,userManager_(userManager)
    ,itemManager_(itemManager)
    ,visitManager_(visitManager)
    ,purchaseManager_(purchaseManager)
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
        // not return false here, if the user id was once removed in userManager_,
        // it would still exists in userIdGenerator_
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

bool RecommendTaskService::visitItem(const std::string& userIdStr, const std::string& itemIdStr)
{
    if (userIdStr.empty() || itemIdStr.empty())
    {
        return false;
    }

    userid_t userId = 0;
    if (userIdGenerator_->conv(userIdStr, userId, false) == false)
    {
        LOG(ERROR) << "error in visitItem(), user id " << userIdStr << " not yet added before";
        return false;
    }

    itemid_t itemId = 0;
    if (itemIdGenerator_->conv(itemIdStr, itemId, false) == false)
    {
        LOG(ERROR) << "error in visitItem(), item id " << itemIdStr << " not yet added before";
        return false;
    }

    return visitManager_->addVisitItem(userId, itemId);
}

bool RecommendTaskService::purchaseItem(
    const std::string& userIdStr,
    const OrderItemVec& orderItemVec,
    const std::string& orderIdStr
)
{
    if (userIdStr.empty())
    {
        return false;
    }

    userid_t userId = 0;
    if (userIdGenerator_->conv(userIdStr, userId, false) == false)
    {
        LOG(ERROR) << "error in purchaseItem(), user id " << userIdStr << " not yet added before";
        return false;
    }

    PurchaseManager::OrderItemVec newOrderItemVec;
    for (OrderItemVec::const_iterator it = orderItemVec.begin();
        it != orderItemVec.end(); ++it)
    {
        if (it->itemIdStr_.empty())
        {
            return false;
        }

        itemid_t itemId = 0;
        if (itemIdGenerator_->conv(it->itemIdStr_, itemId, false) == false)
        {
            LOG(ERROR) << "error in purchaseItem(), item id " << it->itemIdStr_ << " not yet added before";
            return false;
        }

        newOrderItemVec.push_back(PurchaseManager::OrderItem(itemId, it->quantity_, it->price_));
    }

    return purchaseManager_->addPurchaseItem(userId, newOrderItemVec, orderIdStr);
}

bool RecommendTaskService::buildCollection()
{
    LOG(INFO) << "=> RecommendTaskService::buildCollection()";
    LOG(INFO) << "user scd path: " << bundleConfig_->userSCDPath();
    return true;
}

} // namespace sf1r
