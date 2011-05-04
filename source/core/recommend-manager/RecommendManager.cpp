#include "RecommendManager.h"
#include "ItemManager.h"
#include <recommend-manager/ItemFilter.h>

#include <glog/logging.h>

namespace sf1r
{
RecommendManager::RecommendManager(
    ItemManager* itemManager,
    CoVisitManager* coVisitManager,
    ItemCFManager* itemCFManager
)
    : itemManager_(itemManager)
    , coVisitManager_(coVisitManager)
    , itemCFManager_(itemCFManager)
{
}

bool RecommendManager::recommend(
    RecommendType type,
    int maxRecNum,
    userid_t userId,
    const std::vector<itemid_t>& inputItemVec,
    const std::vector<itemid_t>& includeItemVec,
    const std::vector<itemid_t>& excludeItemVec,
    /*const std::vector<ItemCategory>& filterCategoryVec,*/
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
    std::vector<itemid_t> filterItemVec(includeItemVec);
    filterItemVec.insert(filterItemVec.end(), excludeItemVec.begin(), excludeItemVec.end());
    ItemFilter filter(itemManager_, filterItemVec);

    typedef std::list<idmlib::recommender::RecommendedItem> RecItemList;

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

        RecItemList topItems;
        itemCFManager_->getTopItems(maxRecNum, inputItemVec, topItems, &filter);

        for (RecItemList::const_iterator it = topItems.begin();
            it != topItems.end(); ++it)
        {
            recItemVec.push_back(it->itemId);
            recWeightVec.push_back(it->value);
        }
    }
    else if (type == BASED_ON_PURCHASE_HISTORY)
    {
        if (userId == 0)
        {
            LOG(ERROR) << "userId should be positive";
            return false;
        }

        RecItemList topItems;
        itemCFManager_->getTopItems(maxRecNum, userId, topItems, &filter);

        for (RecItemList::const_iterator it = topItems.begin();
            it != topItems.end(); ++it)
        {
            recItemVec.push_back(it->itemId);
            recWeightVec.push_back(it->value);
        }
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

} // namespace sf1r
