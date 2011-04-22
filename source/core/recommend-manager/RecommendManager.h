/**
 * @file RecommendManager.h
 * @author Jun Jiang
 * @date 2011-04-15
 */

#ifndef RECOMMEND_MANAGER_H
#define RECOMMEND_MANAGER_H

#include "RecTypes.h"

namespace sf1r
{

class RecommendManager
{
public:
    enum RecommendType
    {
        FREQUENT_BUY_TOGETHER = 0,
        BUY_ALSO_BUY,
        VIEW_ALSO_VIEW,
        BASED_ON_PURCHASE_HISTORY, /// @p userId must be specified
        BASED_ON_BROWSE_HISTORY,
        BASED_ON_SHOP_CART
    };

    bool recommend(
        RecommendType type,
        int maxRecNum,
        const std::vector<itemid_t>& inputItemVec,
        userid_t userId,
        std::vector<itemid_t>& outputRecVec,
        /*const std::vector<ItemCategory>& filterCategoryVec,*/
        const std::vector<itemid_t>& excludeItemVec,
        const std::vector<itemid_t>& includeItemVec
    );
};

} // namespace sf1r

#endif // RECOMMEND_MANAGER_H
