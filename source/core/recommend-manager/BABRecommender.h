/**
 * @file BABRecommender.h
 * @brief recommend "bought also bought" items
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef BAB_RECOMMENDER_H
#define BAB_RECOMMENDER_H

#include "RecTypes.h"
#include "RecommendItem.h"

#include <vector>

namespace sf1r
{
class ItemFilter;

class BABRecommender
{
public:
    BABRecommender(ItemCFManager* itemCFManager);

    bool recommend(
        int maxRecNum,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

private:
    ItemCFManager* itemCFManager_;
};

} // namespace sf1r

#endif // BAB_RECOMMENDER_H
