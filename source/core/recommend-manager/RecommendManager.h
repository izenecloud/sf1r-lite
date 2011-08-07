/**
 * @file RecommendManager.h
 * @author Jun Jiang
 * @date 2011-04-15
 */

#ifndef RECOMMEND_MANAGER_H
#define RECOMMEND_MANAGER_H

#include "RecTypes.h"

#include <vector>

namespace sf1r
{
class ItemManager;
class VisitManager;
class PurchaseManager;
class CartManager;
class OrderManager;
class ItemCondition;
class ItemFilter;

class RecommendManager
{
public:
    RecommendManager(
        ItemManager* itemManager,
        VisitManager* visitManager,
        PurchaseManager* purchaseManager,
        CartManager* cartManager,
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
        idmlib::recommender::RecommendItemVec& recItemVec
    );

    bool topItemBundle(
        int maxRecNum,
        int minFreq,
        std::vector<vector<itemid_t> >& bundleVec,
        std::vector<int>& freqVec
    );

private:
    bool recommend_vav_(
        int maxRecNum,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        idmlib::recommender::RecommendItemVec& recItemVec
    );

    bool recommend_bab_(
        int maxRecNum,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        idmlib::recommender::RecommendItemVec& recItemVec
    );

    bool recommend_bop_(
        int maxRecNum,
        userid_t userId,
        ItemFilter& filter,
        idmlib::recommender::RecommendItemVec& recItemVec
    );

    bool recommend_bob_(
        int maxRecNum,
        userid_t userId,
        const std::string& sessionIdStr,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        idmlib::recommender::RecommendItemVec& recItemVec
    );

    bool recommend_bos_(
        int maxRecNum,
        userid_t userId,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        idmlib::recommender::RecommendItemVec& recItemVec
    );

    bool recommend_fbt_(
        int maxRecNum,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        idmlib::recommender::RecommendItemVec& recItemVec
    );

    /**
     * recommend user with @p inputItemVec as input items,
     * and filter out the purchased items by @p userId.
     */
    bool recUserByItem_(
        int maxRecNum,
        userid_t userId,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        idmlib::recommender::RecommendItemVec& recItemVec
    );

private:
    ItemManager* itemManager_;
    VisitManager* visitManager_;
    PurchaseManager* purchaseManager_;
    CartManager* cartManager_;
    CoVisitManager* coVisitManager_;
    ItemCFManager* itemCFManager_;
    OrderManager* orderManager_;
};

} // namespace sf1r

#endif // RECOMMEND_MANAGER_H
