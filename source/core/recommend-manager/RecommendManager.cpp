#include "RecommendManager.h"
#include "ItemManager.h"
#include "VisitManager.h"
#include "OrderManager.h"
#include "ItemCondition.h"
#include <recommend-manager/ItemFilter.h>

#include <glog/logging.h>

#include <list>
#include <cassert>

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
    idmlib::recommender::RecommendItemVec& recItemVec
)
{
    if (maxRecNum <= 0)
    {
        LOG(ERROR) << "maxRecNum should be positive";
        return false;
    }

    // insert include items at the front of result
    int includeNum = includeItemVec.size();
    if (includeNum >= maxRecNum)
    {
        includeNum = maxRecNum;
    }
    recItemVec.clear();
    idmlib::recommender::RecommendItem recItem;
    recItem.weight_ = 1;
    for (int i = 0; i < includeNum; ++i)
    {
        recItem.itemId_ = includeItemVec[i];
        recItemVec.push_back(recItem);
    }

    // whether need to recommend
    maxRecNum -= includeNum;
    assert(maxRecNum >= 0);
    if (maxRecNum == 0)
    {
        return true;
    }

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

        std::vector<itemid_t> results;
        coVisitManager_->getCoVisitation(maxRecNum, inputItemVec[0], results, &filter);

        for (std::vector<itemid_t>::const_iterator it = results.begin();
            it != results.end(); ++it)
        {
            recItem.itemId_ = *it;
            recItemVec.push_back(recItem);
        }
    }
    else if (type == BUY_ALSO_BUY)
    {
        if (inputItemVec.empty())
        {
            LOG(ERROR) << "failed to recommend for empty input items";
            return false;
        }

        idmlib::recommender::RecommendItemVec results;
        itemCFManager_->getRecByItem(maxRecNum, inputItemVec, results, &filter);

        recItemVec.insert(recItemVec.end(), results.begin(), results.end());
    }
    else if (type == BASED_ON_PURCHASE_HISTORY)
    {
        if (userId == 0)
        {
            LOG(ERROR) << "userId should be positive";
            return false;
        }

        idmlib::recommender::RecommendItemVec results;
        itemCFManager_->getRecByUser(maxRecNum, userId, results, &filter);

        recItemVec.insert(recItemVec.end(), results.begin(), results.end());
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

        for (std::list<itemid_t>::const_iterator it = results.begin();
            it != results.end(); ++it)
        {
            recItem.itemId_ = *it;
            recItemVec.push_back(recItem);
        }
    }
    else
    {
        LOG(ERROR) << "currently the RecommendType " << type << " is not supported yet";
        return false;
    }

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
