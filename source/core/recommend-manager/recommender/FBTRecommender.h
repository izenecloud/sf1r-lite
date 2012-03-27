/**
 * @file FBTRecommender.h
 * @brief recommend "frequently bought together" items
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef FBT_RECOMMENDER_H
#define FBT_RECOMMENDER_H

#include "Recommender.h"

namespace sf1r
{
class OrderManager;

class FBTRecommender : public Recommender
{
public:
    FBTRecommender(OrderManager& orderManager);

protected:
    virtual bool recommendImpl_(
        RecommendParam& param,
        std::vector<RecommendItem>& recItemVec
    );

private:
    OrderManager& orderManager_;
};

} // namespace sf1r

#endif // FBT_RECOMMENDER_H
