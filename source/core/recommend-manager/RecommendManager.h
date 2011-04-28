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

class RecommendManager
{
public:
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
};

} // namespace sf1r

#endif // RECOMMEND_MANAGER_H
