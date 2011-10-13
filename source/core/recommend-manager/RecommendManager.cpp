#include "RecommendManager.h"
#include "ItemManager.h"
#include "VisitManager.h"
#include "PurchaseManager.h"
#include "CartManager.h"
#include "EventManager.h"
#include "OrderManager.h"
#include "ItemCondition.h"
#include "ItemFilter.h"
#include "BOBRecommender.h"
#include "BOSRecommender.h"
#include "BOERecommender.h"
#include <bundles/recommend/RecommendSchema.h>

#include <glog/logging.h>

#include <list>
#include <cassert>

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
    , cartManager_(cartManager)
    , itemCFManager_(itemCFManager)
    , userEventFilter_(schema, purchaseManager, cartManager, eventManager)
    , tibRecommender_(orderManager)
    , fbtRecommender_(orderManager)
    , vavRecommender_(coVisitManager)
    , babRecommender_(itemCFManager)
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

    recItemVec.clear();
    appendIncludeItems_(maxRecNum, includeItemVec, recItemVec);
    maxRecNum -= recItemVec.size();
    if (maxRecNum == 0)
        return true;

    ItemFilter filter(itemManager_);
    filter.insert(includeItemVec.begin(), includeItemVec.end());
    filter.insert(excludeItemVec.begin(), excludeItemVec.end());
    filter.setCondition(condition);

    bool result = false;
    switch (type)
    {
        case FREQUENT_BUY_TOGETHER:
        {
            result = fbtRecommender_.recommend(maxRecNum, inputItemVec, filter, recItemVec);
            break;
        }

        case BUY_ALSO_BUY:
        {
            result = babRecommender_.recommend(maxRecNum, inputItemVec, filter, recItemVec);
            break;
        }

        case VIEW_ALSO_VIEW:
        {
            result = vavRecommender_.recommend(maxRecNum, inputItemVec, filter, recItemVec);
            break;
        }

        case BASED_ON_EVENT:
        {
            BOERecommender boeRecommender(recommendSchema_, itemCFManager_, userEventFilter_,
                                          userId, maxRecNum, filter);
            result = boeRecommender.recommend(recItemVec);
            break;
        }

        case BASED_ON_BROWSE_HISTORY:
        {
            BOBRecommender bobRecommender(recommendSchema_, itemCFManager_, userEventFilter_,
                                          userId, maxRecNum, filter, visitManager_);
            result = bobRecommender.recommend(sessionIdStr, inputItemVec, recItemVec);
            break;
        }

        case BASED_ON_SHOP_CART:
        {
            BOSRecommender bosRecommender(recommendSchema_, itemCFManager_, userEventFilter_,
                                          userId, maxRecNum, filter, cartManager_);
            result = bosRecommender.recommend(inputItemVec, recItemVec);
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

void RecommendManager::appendIncludeItems_(
    int maxRecNum,
    const std::vector<itemid_t>& includeItemVec,
    std::vector<RecommendItem>& recItemVec
) const
{
    int includeNum = includeItemVec.size();
    if (includeNum >= maxRecNum)
    {
        includeNum = maxRecNum;
    }

    RecommendItem recItem;
    recItem.weight_ = 1;

    for (int i = 0; i < includeNum; ++i)
    {
        recItem.itemId_ = includeItemVec[i];
        recItemVec.push_back(recItem);
    }
}

bool RecommendManager::topItemBundle(
    int maxRecNum,
    int minFreq,
    std::vector<vector<itemid_t> >& bundleVec,
    std::vector<int>& freqVec
)
{
    return tibRecommender_.recommend(maxRecNum, minFreq, bundleVec, freqVec);
}

} // namespace sf1r
