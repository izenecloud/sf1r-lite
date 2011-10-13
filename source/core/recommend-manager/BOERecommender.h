/**
 * @file BOERecommender.h
 * @brief recommend items "based on event"
 * @author Jun Jiang
 * @date 2011-10-13
 */

#ifndef BOE_RECOMMENDER_H
#define BOE_RECOMMENDER_H

#include "UserBaseRecommender.h"
#include "UserEventFilter.h"

namespace sf1r
{

class BOERecommender : public UserBaseRecommender 
{
public:
    BOERecommender(
        const RecommendSchema& schema,
        ItemCFManager* itemCFManager,
        const UserEventFilter& userEventFilter,
        userid_t userId,
        int maxRecNum,
        ItemFilter& filter
    );

    bool recommend(std::vector<RecommendItem>& recItemVec);

private:
    void setReasonEvent_(
        std::vector<RecommendItem>& recItemVec,
        const UserEventFilter::ItemEventMap& itemEventMap
    ) const;
};

} // namespace sf1r

#endif // BOE_RECOMMENDER_H
