/**
 * @file ItemCFRecommender.h
 * @brief recommend items using item-based Collaborative Filtering algorithm
 * @author Jun Jiang
 * @date 2011-10-14
 */

#ifndef ITEM_CF_RECOMMENDER_H
#define ITEM_CF_RECOMMENDER_H

#include "Recommender.h"

#include <vector>
#include <string>

namespace sf1r
{
class UserEventFilter;
class ItemFilter;

class ItemCFRecommender : public Recommender
{
public:
    ItemCFRecommender(
        ItemManager& itemManager,
        ItemCFManager& itemCFManager,
        const UserEventFilter& userEventFilter
    );

protected:
    void recommendItemCF_(
        RecommendParam& param,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

    void setReasonEvent_(
        std::vector<RecommendItem>& recItemVec,
        const std::string& eventName
    ) const;

protected:
    ItemCFManager& itemCFManager_;
    const UserEventFilter& userEventFilter_;
};

} // namespace sf1r

#endif // ITEM_CF_RECOMMENDER_H
