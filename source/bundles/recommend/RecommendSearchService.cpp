#include "RecommendSearchService.h"
#include <recommend-manager/User.h>
#include <recommend-manager/ItemCondition.h>
#include <recommend-manager/UserManager.h>
#include <recommend-manager/ItemManager.h>
#include <recommend-manager/RecommenderManager.h>
#include <recommend-manager/RecommendParam.h>

#include <glog/logging.h>

namespace sf1r
{

RecommendSearchService::RecommendSearchService(
    UserManager* userManager,
    ItemManager* itemManager,
    RecommenderManager* recommenderManager,
    RecIdGenerator* userIdGenerator,
    RecIdGenerator* itemIdGenerator
)
    :userManager_(userManager)
    ,itemManager_(itemManager)
    ,recommenderManager_(recommenderManager)
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

    if (!recommenderManager_->recommend(param, recItemVec))
    {
        LOG(ERROR) << "error in RecommenderManager::recommend()";
        return false;
    }

    if (!getItems_(recItemVec))
        return false;

    return true;
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

bool RecommendSearchService::getItems_(std::vector<RecommendItem>& recItemVec) const
{
    for (std::vector<RecommendItem>::iterator it = recItemVec.begin();
        it != recItemVec.end(); ++it)
    {
        if (!itemManager_->getItem(it->itemId_, it->item_))
        {
            LOG(ERROR) << "error in ItemManager::getItem(), item id: " << it->itemId_;
            return false;
        }

        std::vector<ReasonItem>& reasonItems = it->reasonItems_;
        for (std::vector<ReasonItem>::iterator reasonIt = reasonItems.begin();
            reasonIt != reasonItems.end(); ++reasonIt)
        {
            if (!itemManager_->getItem(reasonIt->itemId_, reasonIt->item_))
            {
                LOG(ERROR) << "error in ItemManager::getItem(), item id: " << reasonIt->itemId_;
                return false;
            }
        }
    }

    return true;
}

bool RecommendSearchService::topItemBundle(
    int maxRecNum,
    int minFreq,
    std::vector<vector<Item> >& bundleVec,
    std::vector<int>& freqVec
)
{
    std::vector<vector<itemid_t> > bundleIdVec;
    if (!recommenderManager_->topItemBundle(maxRecNum, minFreq,
                                          bundleIdVec, freqVec))
    {
        LOG(ERROR) << "error in RecommenderManager::topItemBundle()";
        return false;
    }

    bundleVec.resize(bundleIdVec.size());
    for (std::size_t i = 0; i < bundleIdVec.size(); ++i)
    {
        const vector<itemid_t>& idVec = bundleIdVec[i];
        vector<Item>& itemVec = bundleVec[i];
        itemVec.resize(idVec.size());

        for (std::size_t j = 0; j < idVec.size(); ++j)
        {
            if (!itemManager_->getItem(idVec[j], itemVec[j]))
            {
                LOG(ERROR) << "error in ItemManager::getItem(), item id: " << idVec[j];
                return false;
            }
        }
    }

    return true;
}

} // namespace sf1r
