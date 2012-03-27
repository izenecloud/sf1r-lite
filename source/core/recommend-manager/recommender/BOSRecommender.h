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
class UserEventFilter;
class CartManager;

class BOSRecommender : public ItemCFRecommender 
{
public:
    BOSRecommender(
        ItemCFManager& itemCFManager,
        const UserEventFilter& userEventFilter,
        CartManager& cartManager
    );

protected:
    virtual bool recommendImpl_(
        RecommendParam& param,
        std::vector<RecommendItem>& recItemVec
    );

private:
    bool getCartItems_(RecommendParam& param) const;

private:
    const UserEventFilter& userEventFilter_;
    CartManager& cartManager_;
};

} // namespace sf1r

#endif // BOS_RECOMMENDER_H
