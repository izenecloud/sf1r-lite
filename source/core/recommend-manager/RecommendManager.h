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

class RecommendManager
{
public:
    RecommendManager(
        ItemManager* itemManager,
        VisitManager* visitManager,
        CoVisitManager* coVisitManager,
        ItemCFManager* itemCFManager
    );

    bool recommend(
        RecommendType type,
        int maxRecNum,
        userid_t userId,
        const std::vector<itemid_t>& inputItemVec,
        const std::vector<itemid_t>& includeItemVec,
        const std::vector<itemid_t>& excludeItemVec,
        /*const std::vector<ItemCategory>& filterCategoryVec,*/
        std::vector<itemid_t>& recItemVec,
        std::vector<double>& recWeightVec
    );

private:
    ItemManager* itemManager_;
    VisitManager* visitManager_;
    CoVisitManager* coVisitManager_;
    ItemCFManager* itemCFManager_;
};

} // namespace sf1r

#endif // RECOMMEND_MANAGER_H
