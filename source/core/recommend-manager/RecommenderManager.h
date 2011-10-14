/**
 * @file RecommenderManager.h
 * @author Jun Jiang
 * @date 2011-04-15
 */

#ifndef RECOMMEND_MANAGER_H
#define RECOMMEND_MANAGER_H

#include "RecTypes.h"
#include "RecommendItem.h"
#include "TIBRecommender.h"
#include "RecommenderFactory.h"

#include <vector>

namespace sf1r
{
class ItemManager;
class VisitManager;
class PurchaseManager;
class CartManager;
class EventManager;
class OrderManager;
class ItemCondition;
class ItemFilter;
struct RecommendParam;

class RecommenderManager
{
public:
    RecommenderManager(
        ItemManager* itemManager,
        VisitManager* visitManager,
        PurchaseManager* purchaseManager,
        CartManager* cartManager,
        EventManager* eventManager,
        CoVisitManager* coVisitManager,
        ItemCFManager* itemCFManager,
        OrderManager* orderManager
    );

    bool recommend(
        RecommendParam& param,
        std::vector<RecommendItem>& recItemVec
    );

    bool topItemBundle(
        int maxRecNum,
        int minFreq,
        std::vector<vector<itemid_t> >& bundleVec,
        std::vector<int>& freqVec
    );

private:
    void appendIncludeItems_(
        RecommendParam& param,
        std::vector<RecommendItem>& recItemVec
    ) const;

private:
    TIBRecommender tibRecommender_;
    RecommenderFactory recommenderFactory_;
};

} // namespace sf1r

#endif // RECOMMEND_MANAGER_H
