#include "RecommendManager.h"
#include <bundles/recommend/RecommendSchema.h>
#include "ItemManager.h"
#include "VisitManager.h"
#include "PurchaseManager.h"
#include "CartManager.h"
#include "EventManager.h"
#include "OrderManager.h"
#include "ItemCondition.h"
#include <recommend-manager/ItemFilter.h>

#include <glog/logging.h>

#include <list>
#include <cassert>

namespace
{
const char* EVENT_PURCHASE = "purchase";
const char* EVENT_BROWSE = "browse";
const char* EVENT_CART = "shopping_cart";

void setReasonEvent(
    std::vector<sf1r::RecommendItem>& recItemVec,
    const char* eventName
)
{
    for (std::vector<sf1r::RecommendItem>::iterator recIt = recItemVec.begin();
        recIt != recItemVec.end(); ++recIt)
    {
        for (std::vector<sf1r::ReasonItem>::iterator reasonIt = recIt->reasonItems_.begin();
            reasonIt != recIt->reasonItems_.end(); ++reasonIt)
        {
            reasonIt->event_ = eventName;
        }
    }
}

}

namespace sf1r
{
RecommendManager::RecommendManager(
    const RecommendSchema& schema,
    ItemManager* itemManager,
    VisitManager* visitManager,
    PurchaseManager* purchaseManager,
    CartManager* cartManager,
    EventManager* eventManager,
    CoVisitManager* coVisitManager,
    ItemCFManager* itemCFManager,
    OrderManager* orderManager
)
    : recommendSchema_(schema)
    , itemManager_(itemManager)
    , visitManager_(visitManager)
    , purchaseManager_(purchaseManager)
    , cartManager_(cartManager)
    , eventManager_(eventManager)
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
    std::vector<RecommendItem>& recItemVec
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
    RecommendItem recItem;
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

        case BASED_ON_EVENT:
        {
            result = recommend_boe_(maxRecNum, userId, filter, recItemVec);
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
    std::vector<RecommendItem>& recItemVec
)
{
    if (inputItemVec.empty())
    {
        LOG(ERROR) << "failed to recommend for empty input items";
        return false;
    }

    std::vector<itemid_t> results;
    coVisitManager_->getCoVisitation(maxRecNum, inputItemVec[0], results, &filter);

    RecommendItem recItem;
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
    std::vector<RecommendItem>& recItemVec
)
{
    if (inputItemVec.empty())
    {
        LOG(ERROR) << "failed to recommend for empty input items";
        return false;
    }

    idmlib::recommender::RecommendItemVec results;
    itemCFManager_->recommend(maxRecNum, inputItemVec, results, &filter);

    recItemVec.insert(recItemVec.end(), results.begin(), results.end());

    setReasonEvent(recItemVec, EVENT_PURCHASE);

    return true;
}

bool RecommendManager::recommend_boe_(
    int maxRecNum,
    userid_t userId,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    if (userId == 0)
    {
        LOG(ERROR) << "userId should be positive";
        return false;
    }

    std::vector<itemid_t> inputItemVec;
    ItemEventMap itemEventMap;
    ItemIdSet notRecInputSet;

    if (addUserEvent_(userId, inputItemVec, itemEventMap, filter, notRecInputSet) == false)
    {
        LOG(ERROR) << "failed to add user event for user id " << userId;
        return false;
    }

    // get recommend result
    recByItem_(maxRecNum, inputItemVec, notRecInputSet, filter, recItemVec);

    // set event type
    for (std::vector<sf1r::RecommendItem>::iterator recIt = recItemVec.begin();
        recIt != recItemVec.end(); ++recIt)
    {
        for (std::vector<sf1r::ReasonItem>::iterator reasonIt = recIt->reasonItems_.begin();
            reasonIt != recIt->reasonItems_.end(); ++reasonIt)
        {
            reasonIt->event_ = itemEventMap[reasonIt->itemId_];
        }
    }

    return true;
}

bool RecommendManager::recommend_bob_(
    int maxRecNum,
    userid_t userId,
    const std::string& sessionIdStr,
    const std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    bool result = false;
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
            result = recByUser_(maxRecNum, userId, browseItemVec, filter, recItemVec);
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
        result = recByUser_(maxRecNum, userId, inputItemVec, filter, recItemVec);
    }

    if (result)
    {
        setReasonEvent(recItemVec, EVENT_BROWSE);
    }

    return result;
}

bool RecommendManager::recommend_bos_(
    int maxRecNum,
    userid_t userId,
    const std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    bool result = false;
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

            result = recByUser_(maxRecNum, userId, cartItemVec, filter, recItemVec);
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
        result = recByUser_(maxRecNum, userId, inputItemVec, filter, recItemVec);
    }

    if (result)
    {
        setReasonEvent(recItemVec, EVENT_CART);
    }

    return result;
}

bool RecommendManager::recommend_fbt_(
    int maxRecNum,
    const std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
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

    RecommendItem recItem;
    recItem.weight_ = 1;
    for (std::list<itemid_t>::const_iterator it = results.begin();
        it != results.end(); ++it)
    {
        recItem.itemId_ = *it;
        recItemVec.push_back(recItem);
    }

    return true;
}

bool RecommendManager::recByUser_(
    int maxRecNum,
    userid_t userId,
    const std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    ItemIdSet notRecInputSet;
    if (userId)
    {
        if (filterUserEvent_(userId, filter, notRecInputSet) == false)
        {
            LOG(ERROR) << "failed to filter user event for user id " << userId;
            return false;
        }
    }

    recByItem_(maxRecNum, inputItemVec, notRecInputSet, filter, recItemVec);

    return true;
}

bool RecommendManager::addUserEvent_(
    userid_t userId,
    std::vector<itemid_t>& inputItemVec,
    ItemEventMap& itemEventMap,
    ItemFilter& filter,
    ItemIdSet& notRecInputSet
)
{
    assert(userId);

    // purchase history
    ItemIdSet purchaseItemSet;
    if (purchaseManager_->getPurchaseItemSet(userId, purchaseItemSet) == false)
    {
        LOG(ERROR) << "failed to get purchased items for user id " << userId;
        return false;
    }
    for (ItemIdSet::const_iterator it = purchaseItemSet.begin();
        it != purchaseItemSet.end(); ++it)
    {
        if (itemEventMap.insert(ItemEventMap::value_type(*it, EVENT_PURCHASE)).second)
        {
            inputItemVec.push_back(*it);
        }
    }

    // cart items
    std::vector<itemid_t> cartItemVec;
    if (cartManager_->getCart(userId, cartItemVec) == false)
    {
        LOG(ERROR) << "failed to get shopping cart items for user id " << userId;
        return false;
    }
    for (std::vector<itemid_t>::const_iterator it = cartItemVec.begin();
        it != cartItemVec.end(); ++it)
    {
        if (itemEventMap.insert(ItemEventMap::value_type(*it, EVENT_CART)).second)
        {
            inputItemVec.push_back(*it);
        }
    }

    // user events
    EventManager::EventItemMap eventItemMap;
    if (eventManager_->getEvent(userId, eventItemMap) == false)
    {
        LOG(ERROR) << "failed to get events for user id " << userId;
        return false;
    }
    for (EventManager::EventItemMap::const_iterator eventIt = eventItemMap.begin();
        eventIt != eventItemMap.end(); ++eventIt)
    {
        if (eventIt->first != recommendSchema_.notRecResultEvent_
            && eventIt->first != recommendSchema_.notRecInputEvent_)
        {
            for (ItemIdSet::const_iterator it = eventIt->second.begin();
                it != eventIt->second.end(); ++it)
            {
                if (itemEventMap.insert(ItemEventMap::value_type(*it, eventIt->first)).second)
                {
                    inputItemVec.push_back(*it);
                }
            }
        }
        else
        {
            // events "not_rec_result" or "not_rec_input"
            filter.insert(eventIt->second.begin(), eventIt->second.end());
        }
    }

    // "not_rec_input" event
    notRecInputSet.swap(eventItemMap[recommendSchema_.notRecInputEvent_]);

    return true;
}

bool RecommendManager::filterUserEvent_(
    userid_t userId,
    ItemFilter& filter,
    ItemIdSet& notRecInputSet
)
{
    assert(userId);

    // filter purchase history
    ItemIdSet purchaseItemSet;
    if (purchaseManager_->getPurchaseItemSet(userId, purchaseItemSet) == false)
    {
        LOG(ERROR) << "failed to get purchased items for user id " << userId;
        return false;
    }
    filter.insert(purchaseItemSet.begin(), purchaseItemSet.end());

    // filter cart items
    std::vector<itemid_t> cartItemVec;
    if (cartManager_->getCart(userId, cartItemVec) == false)
    {
        LOG(ERROR) << "failed to get shopping cart items for user id " << userId;
        return false;
    }
    filter.insert(cartItemVec.begin(), cartItemVec.end());

    // filter all events
    EventManager::EventItemMap eventItemMap;
    if (eventManager_->getEvent(userId, eventItemMap) == false)
    {
        LOG(ERROR) << "failed to get events for user id " << userId;
        return false;
    }
    for (EventManager::EventItemMap::const_iterator it = eventItemMap.begin();
        it != eventItemMap.end(); ++it)
    {
        filter.insert(it->second.begin(), it->second.end());
    }

    // "not_rec_input" event
    notRecInputSet.swap(eventItemMap[recommendSchema_.notRecInputEvent_]);

    return true;
}

void RecommendManager::recByItem_(
    int maxRecNum,
    const std::vector<itemid_t>& inputItemVec,
    const ItemIdSet& notRecInputSet,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    idmlib::recommender::RecommendItemVec results;

    if (notRecInputSet.empty())
    {
        itemCFManager_->recommend(maxRecNum, inputItemVec, results, &filter);
    }
    else
    {
        // exclude "notRecInputSet" in input items
        std::vector<itemid_t> newInputItemVec;
        for (std::vector<itemid_t>::const_iterator it = inputItemVec.begin(); 
            it != inputItemVec.end(); ++it)
        {
            if (notRecInputSet.find(*it) == notRecInputSet.end())
            {
                newInputItemVec.push_back(*it);
            }
        }

        itemCFManager_->recommend(maxRecNum, newInputItemVec, results, &filter);
    }

    recItemVec.insert(recItemVec.end(), results.begin(), results.end());
}

} // namespace sf1r
