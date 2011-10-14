/**
 * @file BOBRecommender.h
 * @brief recommend items "based on browse history"
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef BOB_RECOMMENDER_H
#define BOB_RECOMMENDER_H

#include "ItemCFRecommender.h"

namespace sf1r
{
class VisitManager;

class BOBRecommender : public ItemCFRecommender 
{
public:
    BOBRecommender(
        ItemManager& itemManager,
        ItemCFManager& itemCFManager,
        const UserEventFilter& userEventFilter,
        VisitManager& visitManager
    );

    bool recommend(RecommendParam& param, std::vector<RecommendItem>& recItemVec);

private:
    bool getBrowseItems_(RecommendParam& param) const;

private:
    VisitManager& visitManager_;
};

} // namespace sf1r

#endif // BOB_RECOMMENDER_H
