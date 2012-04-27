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
class UserEventFilter;
class VisitManager;

class BOBRecommender : public ItemCFRecommender 
{
public:
    BOBRecommender(
        GetRecommendBase& getRecommendBase,
        const UserEventFilter& userEventFilter,
        VisitManager& visitManager
    );

protected:
    virtual bool recommendImpl_(
        RecommendParam& param,
        std::vector<RecommendItem>& recItemVec
    );

private:
    bool getBrowseItems_(RecommendParam& param) const;

private:
    const UserEventFilter& userEventFilter_;
    VisitManager& visitManager_;
};

} // namespace sf1r

#endif // BOB_RECOMMENDER_H
