#include "RecommendSearchService.h"
#include <recommend-manager/User.h>
#include <recommend-manager/ItemCondition.h>
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
    const std::string& sessionIdStr,
    const std::vector<std::string>& inputItemVec,
    const std::vector<std::string>& includeItemVec,
    const std::vector<std::string>& excludeItemVec,
    const ItemCondition& condition,
    std::vector<RecommendItem>& recItemVec
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

    idmlib::recommender::RecommendItemVec recIdVec;
    if (!recommendManager_->recommend(recType, maxRecNum,
                                      userId, sessionIdStr,
                                      inputIdVec, includeIdVec, excludeIdVec, condition,
                                      recIdVec))
    {
        LOG(ERROR) << "error in RecommendManager::recommend()";
        return false;
    }

    for (idmlib::recommender::RecommendItemVec::const_iterator it = recIdVec.begin();
        it != recIdVec.end(); ++it)
    {
        recItemVec.push_back(RecommendItem());
        RecommendItem& recItem = recItemVec.back();
        recItem.weight_ = it->weight_;

        if (!itemManager_->getItem(it->itemId_, recItem.item_))
        {
            LOG(ERROR) << "error in ItemManager::getItem(), item id: " << it->itemId_;
            return false;
        }

        const std::vector<itemid_t>& reasonIds = it->reasonItemIds_;
        for (std::vector<itemid_t>::const_iterator reasonIt = reasonIds.begin();
            reasonIt != reasonIds.end(); ++reasonIt)
        {
            recItem.reasonItems_.push_back(Item());
            if (!itemManager_->getItem(*reasonIt, recItem.reasonItems_.back()))
            {
                LOG(ERROR) << "error in ItemManager::getItem(), item id: " << *reasonIt;
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
