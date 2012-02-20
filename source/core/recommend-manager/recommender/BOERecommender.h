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
class UserEventFilter;

class BOERecommender : public ItemCFRecommender 
{
public:
    BOERecommender(
        ItemManager& itemManager,
        ItemCFManager& itemCFManager,
        const UserEventFilter& userEventFilter
    );

protected:
    virtual bool recommendImpl_(
        RecommendParam& param,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

    void convertItemWeight_(
        const std::vector<itemid_t>& inputItemIds,
        const ItemRateMap& itemRateMap,
        ItemWeightMap& itemWeightMap
    ) const;

private:
    const UserEventFilter& userEventFilter_;
};

} // namespace sf1r

#endif // BOE_RECOMMENDER_H
