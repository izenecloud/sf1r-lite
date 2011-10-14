/**
 * @file BOERecommender.h
 * @brief recommend items "based on event"
 * @author Jun Jiang
 * @date 2011-10-13
 */

#ifndef BOE_RECOMMENDER_H
#define BOE_RECOMMENDER_H

#include "ItemCFRecommender.h"

namespace sf1r
{

class BOERecommender : public ItemCFRecommender 
{
public:
    BOERecommender(
        ItemManager& itemManager,
        ItemCFManager& itemCFManager,
        const UserEventFilter& userEventFilter
    );

    bool recommend(RecommendParam& param, std::vector<RecommendItem>& recItemVec);
};

} // namespace sf1r

#endif // BOE_RECOMMENDER_H
