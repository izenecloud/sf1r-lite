#include "RecommendSearchService.h"
#include <recommend-manager/common/User.h>
#include <recommend-manager/common/ItemCondition.h>
#include <recommend-manager/common/RecommendParam.h>
#include <recommend-manager/common/TIBParam.h>
#include <recommend-manager/common/ItemBundle.h>
#include <recommend-manager/item/ItemManager.h>
#include <recommend-manager/item/ItemIdGenerator.h>
#include <recommend-manager/storage/UserManager.h>
#include <recommend-manager/recommender/RecommenderFactory.h>
#include <recommend-manager/recommender/Recommender.h>

#include <glog/logging.h>

namespace sf1r
{

RecommendSearchService::RecommendSearchService(
    UserManager& userManager,
    ItemManager& itemManager,
    RecommenderFactory& recommenderFactory,
    ItemIdGenerator& itemIdGenerator
)
    :userManager_(userManager)
    ,itemManager_(itemManager)
    ,recommenderFactory_(recommenderFactory)
    ,itemIdGenerator_(itemIdGenerator)
{
}

bool RecommendSearchService::getUser(const std::string& userId, User& user)
{
    return userManager_.getUser(userId, user);
}

bool RecommendSearchService::recommend(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    if (!convertIds_(param))
        return false;

    Recommender* recommender = recommenderFactory_.getRecommender(param.type);
    if (recommender && recommender->recommend(param, recItemVec))
        return getRecommendItems_(param, recItemVec);

    return false;
}

bool RecommendSearchService::convertIds_(RecommendParam& param)
{
    return convertItemId_(param.inputItems, param.inputItemIds) &&
           convertItemId_(param.includeItems, param.includeItemIds) &&
           convertItemId_(param.excludeItems, param.excludeItemIds);
}

bool RecommendSearchService::convertItemId_(
    const std::vector<std::string>& inputItemVec,
    std::vector<itemid_t>& outputItemVec
)
{
    itemid_t itemId = 0;

    for (std::size_t i = 0; i < inputItemVec.size(); ++i)
    {
        if (itemIdGenerator_.strIdToItemId(inputItemVec[i], itemId))
        {
            outputItemVec.push_back(itemId);
        }
    }

    return true;
}

bool RecommendSearchService::getRecommendItems_(
    const RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
) const
{
    for (std::vector<RecommendItem>::iterator it = recItemVec.begin();
        it != recItemVec.end(); ++it)
    {
        if (! itemManager_.getItem(it->item_.getId(), param.selectRecommendProps, it->item_))
        {
            LOG(ERROR) << "error in ItemManager::getItem(), item id: " << it->item_.getId();
            return false;
        }

        std::vector<ReasonItem>& reasonItems = it->reasonItems_;
        for (std::vector<ReasonItem>::iterator reasonIt = reasonItems.begin();
            reasonIt != reasonItems.end(); ++reasonIt)
        {
            if (! itemManager_.getItem(reasonIt->item_.getId(), param.selectReasonProps, reasonIt->item_))
            {
                LOG(ERROR) << "error in ItemManager::getItem(), item id: " << reasonIt->item_.getId();
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
    TIBRecommender* recommender = recommenderFactory_.getTIBRecommender();
    if (recommender && recommender->recommend(param, bundleVec))
        return getBundleItems_(param.selectRecommendProps, bundleVec);

    return false;
}

bool RecommendSearchService::getBundleItems_(
    const std::vector<std::string>& selectProps,
    std::vector<ItemBundle>& bundleVec
) const
{
    for (std::vector<ItemBundle>::iterator bundleIt = bundleVec.begin();
        bundleIt != bundleVec.end(); ++bundleIt)
    {
        std::vector<Document>& items = bundleIt->items;

        for (std::vector<Document>::iterator it = items.begin();
            it != items.end(); ++it)
        {
            if (! itemManager_.getItem(it->getId(), selectProps, *it))
            {
                LOG(ERROR) << "error in ItemManager::getItem(), item id: " << it->getId();
                return false;
            }
        }
    }

    return true;
}

RecommendType RecommendSearchService::getRecommendType(const std::string& typeStr) const
{
    return recommenderFactory_.getRecommendType(typeStr);
}

} // namespace sf1r
