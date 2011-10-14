/**
 * @file BOSRecommender.h
 * @brief recommend items "based on shopping cart"
 * @author Jun Jiang
 * @date 2011-10-13
 */

#ifndef BOS_RECOMMENDER_H
#define BOS_RECOMMENDER_H

#include "ItemCFRecommender.h"

namespace sf1r
{
class CartManager;

class BOSRecommender : public ItemCFRecommender 
{
public:
    BOSRecommender(
        ItemManager& itemManager,
        ItemCFManager& itemCFManager,
        const UserEventFilter& userEventFilter,
        CartManager& cartManager
    );

    bool recommend(RecommendParam& param, std::vector<RecommendItem>& recItemVec);

private:
    bool getCartItems_(RecommendParam& param) const;

private:
    CartManager& cartManager_;
};

} // namespace sf1r

#endif // BOS_RECOMMENDER_H
