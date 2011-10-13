/**
 * @file BOSRecommender.h
 * @brief recommend items "based on shopping cart"
 * @author Jun Jiang
 * @date 2011-10-13
 */

#ifndef BOS_RECOMMENDER_H
#define BOS_RECOMMENDER_H

#include "UserBaseRecommender.h"

namespace sf1r
{

class CartManager;

class BOSRecommender : public UserBaseRecommender 
{
public:
    BOSRecommender(
        const RecommendSchema& schema,
        ItemCFManager* itemCFManager,
        const UserEventFilter& userEventFilter,
        userid_t userId,
        int maxRecNum,
        ItemFilter& filter,
        CartManager* cartManager
    );

    bool recommend(
        const std::vector<itemid_t>& inputItemVec,
        std::vector<RecommendItem>& recItemVec
    );

private:
    bool recommendImpl_(
        const std::vector<itemid_t>& inputItemVec,
        std::vector<RecommendItem>& recItemVec
    );

    bool getCartItems_(std::vector<itemid_t>& cartItemVec) const;

private:
    CartManager* cartManager_;
};

} // namespace sf1r

#endif // BOS_RECOMMENDER_H
