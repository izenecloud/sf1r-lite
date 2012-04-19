#include "RecommendSearchService.h"
#include "RecommendBundleConfiguration.h"
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

namespace
{
const std::string DOCID("DOCID");
}

namespace sf1r
{

RecommendSearchService::RecommendSearchService(
    RecommendBundleConfiguration& bundleConfig,
    UserManager& userManager,
    ItemManager& itemManager,
    RecommenderFactory& recommenderFactory,
    ItemIdGenerator& itemIdGenerator
)
    :bundleConfig_(bundleConfig)
    ,userManager_(userManager)
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

    if (bundleConfig_.searchNodeConfig_.isSingleNode_)
        param.enableItemCondition(&itemManager_);

    Recommender* recommender = recommenderFactory_.getRecommender(param.type);

    if (recommender && recommender->recommend(param, recItemVec))
        return getRecommendItems_(param, recItemVec);

    return false;
}

bool RecommendSearchService::convertIds_(RecommendParam& param)
{
    return convertItemId_(param.inputItems, param.inputParam.inputItemIds) &&
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
        itemid_t itemId = it->item_.getId();
        if (! itemManager_.getItem(itemId, param.selectRecommendProps, it->item_))
        {
            LOG(ERROR) << "error in ItemManager::getItem(), item id: " << itemId;
            continue;
        }

        getReasonItems_(it->reasonItems_);
    }

    return true;
}

void RecommendSearchService::getReasonItems_(std::vector<ReasonItem>& reasonItems) const
{
    for (std::vector<ReasonItem>::iterator it = reasonItems.begin();
        it != reasonItems.end(); ++it)
    {
        Document& reasonItem = it->item_;
        std::string strItemId;

        if (! itemIdGenerator_.itemIdToStrId(reasonItem.getId(), strItemId))
            continue;

        izenelib::util::UString ustr(strItemId, izenelib::util::UString::UTF_8);
        reasonItem.property(DOCID) = ustr;
    }
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
