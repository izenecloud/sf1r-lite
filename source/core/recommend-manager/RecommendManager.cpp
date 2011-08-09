#include "RecommendManager.h"
#include "ItemManager.h"
#include "VisitManager.h"
#include "PurchaseManager.h"
#include "CartManager.h"
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
    PurchaseManager* purchaseManager,
    CartManager* cartManager,
    CoVisitManager* coVisitManager,
    ItemCFManager* itemCFManager,
    OrderManager* orderManager
)
    : itemManager_(itemManager)
    , visitManager_(visitManager)
    , purchaseManager_(purchaseManager)
    , cartManager_(cartManager)
    , coVisitManager_(coVisitManager)
    , itemCFManager_(itemCFManager)
    , orderManager_(orderManager)
{
}

bool RecommendManager::recommend(
    RecommendType type,
    int maxRecNum,
    userid_t userId,
    const std::string& sessionIdStr,
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

    bool result = false;
    switch (type)
    {
        case FREQUENT_BUY_TOGETHER:
        {
            result = recommend_fbt_(maxRecNum, inputItemVec, filter, recItemVec);
            break;
        }

        case BUY_ALSO_BUY:
        {
            result = recommend_bab_(maxRecNum, inputItemVec, filter, recItemVec);
            break;
        }

        case VIEW_ALSO_VIEW:
        {
            result = recommend_vav_(maxRecNum, inputItemVec, filter, recItemVec);
            break;
        }

        case BASED_ON_PURCHASE_HISTORY:
        {
            result = recommend_bop_(maxRecNum, userId, filter, recItemVec);
            break;
        }

        case BASED_ON_BROWSE_HISTORY:
        {
            result = recommend_bob_(maxRecNum, userId, sessionIdStr, inputItemVec, filter, recItemVec);
            break;
        }

        case BASED_ON_SHOP_CART:
        {
            result = recommend_bos_(maxRecNum, userId, inputItemVec, filter, recItemVec);
            break;
        }

        default:
        {
            LOG(ERROR) << "currently the RecommendType " << type << " is not supported yet";
            result = false;
            break;
        }
    }

    return result;
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

bool RecommendManager::recommend_vav_(
    int maxRecNum,
    const std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    idmlib::recommender::RecommendItemVec& recItemVec
)
{
    if (inputItemVec.empty())
    {
        LOG(ERROR) << "failed to recommend for empty input items";
        return false;
    }

    std::vector<itemid_t> results;
    coVisitManager_->getCoVisitation(maxRecNum, inputItemVec[0], results, &filter);

    idmlib::recommender::RecommendItem recItem;
    recItem.weight_ = 1;
    for (std::vector<itemid_t>::const_iterator it = results.begin();
        it != results.end(); ++it)
    {
        recItem.itemId_ = *it;
        recItemVec.push_back(recItem);
    }

    return true;
}

bool RecommendManager::recommend_bab_(
    int maxRecNum,
    const std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    idmlib::recommender::RecommendItemVec& recItemVec
)
{
    if (inputItemVec.empty())
    {
        LOG(ERROR) << "failed to recommend for empty input items";
        return false;
    }

    idmlib::recommender::RecommendItemVec results;
    itemCFManager_->getRecByItem(maxRecNum, inputItemVec, results, &filter);

    recItemVec.insert(recItemVec.end(), results.begin(), results.end());

    return true;
}

bool RecommendManager::recommend_bop_(
    int maxRecNum,
    userid_t userId,
    ItemFilter& filter,
    idmlib::recommender::RecommendItemVec& recItemVec
)
{
    if (userId == 0)
    {
        LOG(ERROR) << "userId should be positive";
        return false;
    }

    idmlib::recommender::RecommendItemVec results;
    itemCFManager_->getRecByUser(maxRecNum, userId, results, &filter);

    recItemVec.insert(recItemVec.end(), results.begin(), results.end());

    return true;
}

bool RecommendManager::recommend_bob_(
    int maxRecNum,
    userid_t userId,
    const std::string& sessionIdStr,
    const std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    idmlib::recommender::RecommendItemVec& recItemVec
)
{
    if (inputItemVec.empty())
    {
        // use visit session items as browse history
        if (userId && sessionIdStr.empty() == false)
        {
            VisitSession visitSession;
            if (visitManager_->getVisitSession(userId, visitSession) == false)
            {
                LOG(ERROR) << "failed to get visit session items for user id " << userId;
                return false;
            }

            std::vector<itemid_t> browseItemVec;
            if (visitSession.sessionId_ == sessionIdStr)
            {
                browseItemVec.assign(visitSession.itemSet_.begin(), visitSession.itemSet_.end());
            }
            return recUserByItem_(maxRecNum, userId, browseItemVec, filter, recItemVec);
        }
        else
        {
            LOG(ERROR) << "failed to recommend with empty browse history";
            return false;
        }
    }
    else
    {
        // use input items as browse history
        return recUserByItem_(maxRecNum, userId, inputItemVec, filter, recItemVec);
    }
}

bool RecommendManager::recommend_bos_(
    int maxRecNum,
    userid_t userId,
    const std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    idmlib::recommender::RecommendItemVec& recItemVec
)
{
    if (inputItemVec.empty())
    {
        // use cart items as shopping cart
        if (userId)
        {
            std::vector<itemid_t> cartItemVec;
            if (cartManager_->getCart(userId, cartItemVec) == false)
            {
                LOG(ERROR) << "failed to get shopping cart items for user id " << userId;
                return false;
            }

            return recUserByItem_(maxRecNum, userId, cartItemVec, filter, recItemVec);
        }
        else
        {
            LOG(ERROR) << "failed to recommend with empty shopping cart and empty user id";
            return false;
        }
    }
    else
    {
        // use input items as shopping cart
        return recUserByItem_(maxRecNum, userId, inputItemVec, filter, recItemVec);
    }
}

bool RecommendManager::recommend_fbt_(
    int maxRecNum,
    const std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    idmlib::recommender::RecommendItemVec& recItemVec
)
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

    idmlib::recommender::RecommendItem recItem;
    recItem.weight_ = 1;
    for (std::list<itemid_t>::const_iterator it = results.begin();
        it != results.end(); ++it)
    {
        recItem.itemId_ = *it;
        recItemVec.push_back(recItem);
    }

    return true;
}

bool RecommendManager::recUserByItem_(
    int maxRecNum,
    userid_t userId,
    const std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    idmlib::recommender::RecommendItemVec& recItemVec
)
{
    // filter purchase history
    if (userId)
    {
        ItemIdSet purchaseItemSet;
        if (purchaseManager_->getPurchaseItemSet(userId, purchaseItemSet) == false)
        {
            LOG(ERROR) << "failed to get purchased items for user id " << userId;
            return false;
        }
        filter.insert(purchaseItemSet.begin(), purchaseItemSet.end());
    }

    idmlib::recommender::RecommendItemVec results;
    itemCFManager_->getRecByItem(maxRecNum, inputItemVec, results, &filter);

    recItemVec.insert(recItemVec.end(), results.begin(), results.end());

    return true;
}

} // namespace sf1r
