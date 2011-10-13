/**
 * @file FBTRecommender.h
 * @brief recommend "frequently bought together" items
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef FBT_RECOMMENDER_H
#define FBT_RECOMMENDER_H

#include "RecTypes.h"
#include "RecommendItem.h"

#include <vector>

namespace sf1r
{
class OrderManager;
class ItemFilter;

class FBTRecommender
{
public:
    FBTRecommender(OrderManager* orderManager);

    bool recommend(
        int maxRecNum,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

private:
    OrderManager* orderManager_;
};

} // namespace sf1r

#endif // FBT_RECOMMENDER_H
