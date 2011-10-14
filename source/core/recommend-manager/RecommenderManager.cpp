#include "RecommenderManager.h"
#include "ItemManager.h"
#include "VisitManager.h"
#include "PurchaseManager.h"
#include "CartManager.h"
#include "EventManager.h"
#include "OrderManager.h"
#include "ItemCondition.h"
#include "ItemFilter.h"
#include "RecommendParam.h"

#include <glog/logging.h>

namespace sf1r
{
RecommenderManager::RecommenderManager(
    ItemManager* itemManager,
    VisitManager* visitManager,
    PurchaseManager* purchaseManager,
    CartManager* cartManager,
    EventManager* eventManager,
    CoVisitManager* coVisitManager,
    ItemCFManager* itemCFManager,
    OrderManager* orderManager
)
    : tibRecommender_(*orderManager)
    , recommenderFactory_(*itemManager, *visitManager, *purchaseManager, *cartManager,
                          *orderManager, *eventManager, *coVisitManager, *itemCFManager)
{
}

bool RecommenderManager::recommend(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    recItemVec.clear();

    if (param.limit <= 0)
    {
        LOG(ERROR) << "recommend count should be positive";
        return false;
    }

    appendIncludeItems_(param, recItemVec);
    if (param.limit == 0)
        return true;

    Recommender* recommender = recommenderFactory_.getRecommender(param.type);
    if (recommender)
        return recommender->recommend(param, recItemVec);

    return false;
}

void RecommenderManager::appendIncludeItems_(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
) const
{
    int includeNum = param.includeItemIds.size();
    if (includeNum >= param.limit)
    {
        includeNum = param.limit;
    }

    RecommendItem recItem;
    recItem.weight_ = 1;

    for (int i = 0; i < includeNum; ++i)
    {
        recItem.itemId_ = param.includeItemIds[i];
        recItemVec.push_back(recItem);
    }

    param.limit -= recItemVec.size();
}

bool RecommenderManager::topItemBundle(
    int maxRecNum,
    int minFreq,
    std::vector<vector<itemid_t> >& bundleVec,
    std::vector<int>& freqVec
)
{
    return tibRecommender_.recommend(maxRecNum, minFreq, bundleVec, freqVec);
}

} // namespace sf1r
