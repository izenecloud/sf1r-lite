/**
 * @file RecommendManager.h
 * @author Jun Jiang
 * @date 2011-04-15
 */

#ifndef RECOMMEND_MANAGER_H
#define RECOMMEND_MANAGER_H

#include "RecTypes.h"
#include "RecommendItem.h"
#include "TIBRecommender.h"
#include "FBTRecommender.h"
#include "VAVRecommender.h"
#include "BABRecommender.h"
#include "UserEventFilter.h"

#include <vector>

namespace sf1r
{
class RecommendSchema;
class ItemManager;
class VisitManager;
class PurchaseManager;
class CartManager;
class EventManager;
class OrderManager;
class ItemCondition;
class ItemFilter;

class RecommendManager
{
public:
    RecommendManager(
        const RecommendSchema& schema,
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
        RecommendType type,
        int maxRecNum,
        userid_t userId,
        const std::string& sessionIdStr,
        const std::vector<itemid_t>& inputItemVec,
        const std::vector<itemid_t>& includeItemVec,
        const std::vector<itemid_t>& excludeItemVec,
        const ItemCondition& condition,
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
        int maxRecNum,
        const std::vector<itemid_t>& includeItemVec,
        std::vector<RecommendItem>& recItemVec
    ) const;

private:
    const RecommendSchema& recommendSchema_;
    ItemManager* itemManager_;
    VisitManager* visitManager_;
    CartManager* cartManager_;
    ItemCFManager* itemCFManager_;

    UserEventFilter userEventFilter_;
    TIBRecommender tibRecommender_;
    FBTRecommender fbtRecommender_;
    VAVRecommender vavRecommender_;
    BABRecommender babRecommender_;
};

} // namespace sf1r

#endif // RECOMMEND_MANAGER_H
