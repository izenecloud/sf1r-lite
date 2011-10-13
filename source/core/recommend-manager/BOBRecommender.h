/**
 * @file BOBRecommender.h
 * @brief recommend items "based on browse history"
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef BOB_RECOMMENDER_H
#define BOB_RECOMMENDER_H

#include "UserBaseRecommender.h"

#include <string>
#include <vector>

namespace sf1r
{

class VisitManager;

class BOBRecommender : public UserBaseRecommender 
{
public:
    BOBRecommender(
        const RecommendSchema& schema,
        ItemCFManager* itemCFManager,
        const UserEventFilter& userEventFilter,
        userid_t userId,
        int maxRecNum,
        ItemFilter& filter,
        VisitManager* visitManager
    );

    bool recommend(
        const std::string& sessionIdStr,
        const std::vector<itemid_t>& inputItemVec,
        std::vector<RecommendItem>& recItemVec
    );

private:
    bool getBrowseItems_(
        const std::string& sessionIdStr,
        std::vector<itemid_t>& browseItemVec
    ) const;

    bool recommendImpl_(
        const std::vector<itemid_t>& inputItemVec,
        std::vector<RecommendItem>& recItemVec
    );

private:
    VisitManager* visitManager_;
};

} // namespace sf1r

#endif // BOB_RECOMMENDER_H
