/**
 * @file UserBaseRecommender.h
 * @brief class to recommend items for a user
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef USER_BASE_RECOMMENDER_H
#define USER_BASE_RECOMMENDER_H

#include "RecTypes.h"
#include "RecommendItem.h"

#include <vector>
#include <string>

namespace sf1r
{
class RecommendSchema;
class UserEventFilter;
class ItemFilter;

class UserBaseRecommender
{
public:
    UserBaseRecommender(
        const RecommendSchema& schema,
        ItemCFManager* itemCFManager,
        const UserEventFilter& userEventFilter,
        userid_t userId,
        int maxRecNum,
        ItemFilter& filter
    );

protected:
    void recommendByItem_(
        const std::vector<itemid_t>& inputItemVec,
        std::vector<RecommendItem>& recItemVec
    );

    bool filterUserEvent_();

    void setReasonEvent_(
        std::vector<RecommendItem>& recItemVec,
        const std::string& eventName
    ) const;

private:
    void recommendByItemImpl_(
        const std::vector<itemid_t>& inputItemVec,
        std::vector<RecommendItem>& recItemVec
    );

protected:
    const RecommendSchema& recommendSchema_;
    ItemCFManager* itemCFManager_;
    const UserEventFilter& userEventFilter_;

    const userid_t userId_;
    const int maxRecNum_;
    ItemFilter& filter_;

    ItemIdSet notRecInputSet_;
};

} // namespace sf1r

#endif // USER_BASE_RECOMMENDER_H
