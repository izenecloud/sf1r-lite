#include "RecommendManager.h"
#include "ItemManager.h"
#include "VisitManager.h"
#include "OrderManager.h"
#include "ItemCondition.h"
#include <recommend-manager/ItemFilter.h>

#include <glog/logging.h>

#include <list>

namespace sf1r
{
RecommendManager::RecommendManager(
    ItemManager* itemManager,
    VisitManager* visitManager,
    CoVisitManager* coVisitManager,
    ItemCFManager* itemCFManager,
    OrderManager* orderManager
)
    : itemManager_(itemManager)
    , visitManager_(visitManager)
    , coVisitManager_(coVisitManager)
    , itemCFManager_(itemCFManager)
    , orderManager_(orderManager)
{
}

bool RecommendManager::recommend(
    RecommendType type,
    int maxRecNum,
    userid_t userId,
    const std::vector<itemid_t>& inputItemVec,
    const std::vector<itemid_t>& includeItemVec,
    const std::vector<itemid_t>& excludeItemVec,
    const ItemCondition& condition,
    std::vector<itemid_t>& recItemVec,
    std::vector<double>& recWeightVec
)
{
    if (maxRecNum <= 0)
    {
        LOG(ERROR) << "maxRecNum should be positive";
        return false;
    }

    int includeNum = includeItemVec.size();
    if (includeNum >= maxRecNum)
    {
        recItemVec.assign(includeItemVec.begin(), includeItemVec.begin() + maxRecNum);
        recWeightVec.assign(maxRecNum, 1);

        return true;
    }

    maxRecNum -= includeNum;

    // filter both include and exclude items
    ItemFilter filter(itemManager_);
    filter.insert(includeItemVec.begin(), includeItemVec.end());
    filter.insert(excludeItemVec.begin(), excludeItemVec.end());
    filter.setCondition(condition);

    if (type == BASED_ON_BROWSE_HISTORY || type == BASED_ON_SHOP_CART)
    {
        if (userId == 0)
        {
            // for code reuse
            type = BUY_ALSO_BUY;
        }
        else
        {
            filter.insert(inputItemVec.begin(), inputItemVec.end());

            if (type == BASED_ON_BROWSE_HISTORY)
            {
                // filter items in browse history
                ItemIdSet itemIdSet;
                if (!visitManager_->getVisitItemSet(userId, itemIdSet))
                {
                    LOG(ERROR) << "failed to get visited items for user id " << userId;
                    return false;
                }
                filter.insert(itemIdSet.begin(), itemIdSet.end());
            }

            // for code reuse
            type = BASED_ON_PURCHASE_HISTORY;
        }
    }

    if (type == VIEW_ALSO_VIEW)
    {
        if (inputItemVec.empty())
        {
            LOG(ERROR) << "failed to recommend for empty input items";
            return false;
        }

        coVisitManager_->getCoVisitation(maxRecNum, inputItemVec[0], recItemVec, &filter);
        recWeightVec.assign(recItemVec.size(), 1);
    }
    else if (type == BUY_ALSO_BUY)
    {
        if (inputItemVec.empty())
        {
            LOG(ERROR) << "failed to recommend for empty input items";
            return false;
        }

        idmlib::recommender::RecommendItemVec recItems;
        itemCFManager_->getRecByItem(maxRecNum, inputItemVec, recItems, &filter);

        for (idmlib::recommender::RecommendItemVec::const_iterator it = recItems.begin();
            it != recItems.end(); ++it)
        {
            recItemVec.push_back(it->itemId_);
            recWeightVec.push_back(it->weight_);
        }
    }
    else if (type == BASED_ON_PURCHASE_HISTORY)
    {
        if (userId == 0)
        {
            LOG(ERROR) << "userId should be positive";
            return false;
        }

        idmlib::recommender::RecommendItemVec recItems;
        itemCFManager_->getRecByUser(maxRecNum, userId, recItems, &filter);

        for (idmlib::recommender::RecommendItemVec::const_iterator it = recItems.begin();
            it != recItems.end(); ++it)
        {
            recItemVec.push_back(it->itemId_);
            recWeightVec.push_back(it->weight_);
        }
    }
    else if (type == FREQUENT_BUY_TOGETHER)
    {
        if (inputItemVec.empty())
        {
            LOG(ERROR) << "failed to recommend for empty input items";
            return false;
        }

        std::list<itemid_t> inputItemList(inputItemVec.begin(), inputItemVec.end());
        std::list<itemid_t> results;
        if (orderManager_->getFreqItemSets(maxRecNum, inputItemList, results, &filter) == false)
        {
            LOG(ERROR) << "failed in OrderManager::getFreqItemSets()";
            return false;
        }

        recItemVec.assign(results.begin(), results.end());
        recWeightVec.assign(recItemVec.size(), 1);
    }
    else
    {
        LOG(ERROR) << "currently the RecommendType " << type << " is not supported yet";
        return false;
    }

    // insert include items at the front of result
    recItemVec.insert(recItemVec.begin(), includeItemVec.begin(), includeItemVec.begin() + includeNum);
    recWeightVec.insert(recWeightVec.begin(), includeNum, 1);

    return true;
}

bool RecommendManager::topItemBundle(
    int maxRecNum,
    int minFreq,
    std::vector<vector<itemid_t> >& bundleVec,
    std::vector<int>& freqVec
)
{
    FrequentItemSetResultType results;
    orderManager_->getAllFreqItemSets(maxRecNum, minFreq, results);

    const std::size_t resultNum = results.size();
    bundleVec.resize(resultNum);
    freqVec.resize(resultNum);
    for (std::size_t i = 0; i < resultNum; ++i)
    {
        bundleVec[i].swap(results[i].first);
        freqVec[i] = results[i].second;
    }

    return true;
}

} // namespace sf1r
