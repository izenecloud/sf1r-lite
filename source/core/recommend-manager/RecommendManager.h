/**
 * @file RecommendManager.h
 * @author Jun Jiang
 * @date 2011-04-15
 */

#ifndef RECOMMEND_MANAGER_H
#define RECOMMEND_MANAGER_H

#include "RecTypes.h"
#include "RecommendItem.h"

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
    bool recommend_vav_(
        int maxRecNum,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

    bool recommend_bab_(
        int maxRecNum,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

    bool recommend_boe_(
        int maxRecNum,
        userid_t userId,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

    bool recommend_bob_(
        int maxRecNum,
        userid_t userId,
        const std::string& sessionIdStr,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

    bool recommend_bos_(
        int maxRecNum,
        userid_t userId,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

    bool recommend_fbt_(
        int maxRecNum,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

    /**
     * recommend user @p userId with input items @p inputItemVec.
     */
    bool recByUser_(
        int maxRecNum,
        userid_t userId,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

    typedef std::map<itemid_t, std::string> ItemEventMap;
    /**
     * add the purchased items, cart itmes and all events (except events "not_rec_result" and "not_rec_input") into @p inputItemVec and @p itemEventMap,
     * add the items in events "not_rec_result" and "not_rec_input" into @p filter,
     * add the items in "not_rec_input" event into @p notRecInputSet.
     */
    bool addUserEvent_(
        userid_t userId,
        std::vector<itemid_t>& inputItemVec,
        ItemEventMap& itemEventMap,
        ItemFilter& filter,
        ItemIdSet& notRecInputSet
    );

    /**
     * add the purchased items, cart itmes and all events into @p filter,
     * add the items in "not_rec_input" event into @p notRecInputSet.
     */
    bool filterUserEvent_(
        userid_t userId,
        ItemFilter& filter,
        ItemIdSet& notRecInputSet
    );

    /**
     * recommend with input items @p inputItemVec,
     * but without items in @p notRecInputSet,
     * excluding results with @p filter.
     */
    void recByItem_(
        int maxRecNum,
        const std::vector<itemid_t>& inputItemVec,
        const ItemIdSet& notRecInputSet,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

private:
    const RecommendSchema& recommendSchema_;
    ItemManager* itemManager_;
    VisitManager* visitManager_;
    PurchaseManager* purchaseManager_;
    CartManager* cartManager_;
    EventManager* eventManager_;
    CoVisitManager* coVisitManager_;
    ItemCFManager* itemCFManager_;
    OrderManager* orderManager_;
};

} // namespace sf1r

#endif // RECOMMEND_MANAGER_H
