#include "RecommendSearchService.h"
#include <recommend-manager/User.h>
#include <recommend-manager/Item.h>
#include <recommend-manager/UserManager.h>
#include <recommend-manager/ItemManager.h>
#include <recommend-manager/RecommendManager.h>

#include <glog/logging.h>

namespace sf1r
{

RecommendSearchService::RecommendSearchService(
    UserManager* userManager,
    ItemManager* itemManager,
    RecommendManager* recommendManager,
    RecIdGenerator* userIdGenerator,
    RecIdGenerator* itemIdGenerator
)
    :userManager_(userManager)
    ,itemManager_(itemManager)
    ,recommendManager_(recommendManager)
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

bool RecommendSearchService::convertItemId(
    const std::vector<std::string>& inputItemVec,
    std::vector<itemid_t>& outputItemVec
)
{
    itemid_t itemId = 0;

    for (std::size_t i = 0; i < inputItemVec.size(); ++i)
    {
        if (itemIdGenerator_->conv(inputItemVec[i], itemId, false) == false)
        {
            LOG(ERROR) << "error in convertItemId(), item id " << inputItemVec[i] << " not yet added before";
            return false;
        }

        outputItemVec.push_back(itemId);
    }

    return true;
}

bool RecommendSearchService::recommend(
    RecommendType recType,
    int maxRecNum,
    const std::string& userIdStr,
    const std::vector<std::string>& inputItemVec,
    const std::vector<std::string>& includeItemVec,
    const std::vector<std::string>& excludeItemVec,
    std::vector<Item>& recItemVec,
    std::vector<double>& recWeightVec
)
{
    userid_t userId = 0;
    if (!userIdStr.empty() && userIdGenerator_->conv(userIdStr, userId, false) == false)
    {
        LOG(ERROR) << "error in recommend(), user id " << userIdStr << " not yet added before";
        return false;
    }

    std::vector<itemid_t> inputIdVec;
    std::vector<itemid_t> includeIdVec;
    std::vector<itemid_t> excludeIdVec;
    if (!convertItemId(inputItemVec, inputIdVec)
        || !convertItemId(includeItemVec, includeIdVec)
        || !convertItemId(excludeItemVec, excludeIdVec))
    {
        return false;
    }

    std::vector<itemid_t> recIdVec;
    if (!recommendManager_->recommend(recType, maxRecNum, userId,
                             inputIdVec, includeIdVec, excludeIdVec,
                             recIdVec, recWeightVec))
    {
        LOG(ERROR) << "error in RecommendManager::recommend()";
        return false;
    }

    for (std::size_t i = 0; i < recIdVec.size(); ++i)
    {
        recItemVec.push_back(Item());
        if (!itemManager_->getItem(recIdVec[i], recItemVec.back()))
        {
            LOG(ERROR) << "error in ItemManager::getItem(), item id: " << recIdVec[i];
            return false;
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
    if (!recommendManager_->topItemBundle(maxRecNum, minFreq,
                                          bundleIdVec, freqVec))
    {
        LOG(ERROR) << "error in RecommendManager::topItemBundle()";
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
