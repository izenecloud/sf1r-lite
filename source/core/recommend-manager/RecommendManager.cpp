#include "RecommendManager.h"
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

    if (type == VIEW_ALSO_VIEW)
    {
        int includeNum = includeItemVec.size();
        if (includeNum < maxRecNum)
        {
            maxRecNum -= includeNum;

            if (inputItemVec.empty())
            {
                LOG(ERROR) << "failed to recommend for empty input items";
                return false;
            }

            // filter both include and exclude items
            std::vector<itemid_t> filterItemVec(includeItemVec);
            filterItemVec.insert(filterItemVec.end(), excludeItemVec.begin(), excludeItemVec.end());
            ItemFilter filter(itemManager_, filterItemVec);

            coVisitManager_->getCoVisitation(maxRecNum, inputItemVec[0], recItemVec, &filter);
        }
        else
        {
            includeNum = maxRecNum;
        }

        // insert include items at the front of result
        recItemVec.insert(recItemVec.begin(), includeItemVec.begin(), includeItemVec.begin() + includeNum);
        recWeightVec.resize(recItemVec.size(), 1);
    }
    else
    {
        LOG(ERROR) << "currently the RecommendType " << type << " is not supported yet";
        return false;
    }

    return true;
}

} // namespace sf1r
