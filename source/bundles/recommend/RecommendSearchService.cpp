#include "RecommendSearchService.h"
#include <recommend-manager/User.h>
#include <recommend-manager/ItemCondition.h>
#include <recommend-manager/UserManager.h>
#include <recommend-manager/ItemManager.h>
#include <recommend-manager/RecommenderFactory.h>
#include <recommend-manager/RecommendParam.h>
#include <recommend-manager/TIBParam.h>
#include <recommend-manager/ItemBundle.h>

#include <glog/logging.h>

namespace sf1r
{

RecommendSearchService::RecommendSearchService(
    UserManager* userManager,
    ItemManager* itemManager,
    RecommenderFactory* recommenderFactory,
    RecIdGenerator* userIdGenerator,
    RecIdGenerator* itemIdGenerator
)
    :userManager_(userManager)
    ,itemManager_(itemManager)
    ,recommenderFactory_(recommenderFactory)
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

bool RecommendSearchService::recommend(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    if (!convertIds_(param))
        return false;

    Recommender* recommender = recommenderFactory_->getRecommender(param.type);
    if (recommender && recommender->recommend(param, recItemVec))
        return getRecommendItems_(recItemVec);

    return false;
}

bool RecommendSearchService::convertIds_(RecommendParam& param)
{
    if (!param.userIdStr.empty()
        && !userIdGenerator_->conv(param.userIdStr, param.userId, false))
    {
        LOG(ERROR) << "error in convertIds_(), user id " << param.userIdStr << " not yet added before";
        return false;
    }

    if (!convertItemId_(param.inputItems, param.inputItemIds)
        || !convertItemId_(param.includeItems, param.includeItemIds)
        || !convertItemId_(param.excludeItems, param.excludeItemIds))
    {
        return false;
    }

    return true;
}

bool RecommendSearchService::convertItemId_(
    const std::vector<std::string>& inputItemVec,
    std::vector<itemid_t>& outputItemVec
)
{
    itemid_t itemId = 0;

    for (std::size_t i = 0; i < inputItemVec.size(); ++i)
    {
        if (itemIdGenerator_->conv(inputItemVec[i], itemId, false) == false)
        {
            LOG(ERROR) << "error in convertItemId_(), item id " << inputItemVec[i] << " not yet added before";
            return false;
        }

        outputItemVec.push_back(itemId);
    }

    return true;
}

bool RecommendSearchService::getRecommendItems_(std::vector<RecommendItem>& recItemVec) const
{
    for (std::vector<RecommendItem>::iterator it = recItemVec.begin();
        it != recItemVec.end(); ++it)
    {
        if (! itemManager_->getItem(it->itemId_, it->item_))
        {
            LOG(ERROR) << "error in ItemManager::getItem(), item id: " << it->itemId_;
            return false;
        }

        std::vector<ReasonItem>& reasonItems = it->reasonItems_;
        for (std::vector<ReasonItem>::iterator reasonIt = reasonItems.begin();
            reasonIt != reasonItems.end(); ++reasonIt)
        {
            if (! itemManager_->getItem(reasonIt->itemId_, reasonIt->item_))
            {
                LOG(ERROR) << "error in ItemManager::getItem(), item id: " << reasonIt->itemId_;
                return false;
            }
        }
    }

    return true;
}

bool RecommendSearchService::topItemBundle(
    const TIBParam& param,
    std::vector<ItemBundle>& bundleVec
)
{
    TIBRecommender* recommender = recommenderFactory_->getTIBRecommender();
    if (recommender && recommender->recommend(param, bundleVec))
        return getBundleItems_(bundleVec);

    return false;
}

bool RecommendSearchService::getBundleItems_(std::vector<ItemBundle>& bundleVec) const
{
    for (std::vector<ItemBundle>::iterator bundleIt = bundleVec.begin();
        bundleIt != bundleVec.end(); ++bundleIt)
    {
        const std::vector<itemid_t>& itemIds = bundleIt->itemIds;
        std::vector<Item>& items = bundleIt->items;
        items.resize(itemIds.size());

        for (std::size_t i = 0; i < itemIds.size(); ++i)
        {
            if (! itemManager_->getItem(itemIds[i], items[i]))
            {
                LOG(ERROR) << "error in ItemManager::getItem(), item id: " << itemIds[i];
                return false;
            }
        }
    }

    return true;
}

} // namespace sf1r
