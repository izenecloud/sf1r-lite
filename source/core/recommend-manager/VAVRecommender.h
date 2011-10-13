/**
 * @file VAVRecommender.h
 * @brief recommend "viewed also view" items
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef VAV_RECOMMENDER_H
#define VAV_RECOMMENDER_H

#include "RecTypes.h"
#include "RecommendItem.h"

#include <vector>

namespace sf1r
{
class ItemFilter;

class VAVRecommender
{
public:
    VAVRecommender(CoVisitManager* coVisitManager);

    bool recommend(
        int maxRecNum,
        const std::vector<itemid_t>& inputItemVec,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

private:
    CoVisitManager* coVisitManager_;
};

} // namespace sf1r

#endif // VAV_RECOMMENDER_H
