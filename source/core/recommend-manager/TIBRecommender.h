/**
 * @file TIBRecommender.h
 * @brief recommend "top item bundle"
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef TIB_RECOMMENDER_H
#define TIB_RECOMMENDER_H

#include <vector>

namespace sf1r
{
class OrderManager;
struct TIBParam;
struct ItemBundle;

class TIBRecommender
{
public:
    TIBRecommender(OrderManager& orderManager);

    bool recommend(
        const TIBParam& param,
        std::vector<ItemBundle>& bundleVec
    );

private:
    OrderManager& orderManager_;
};

} // namespace sf1r

#endif // TIB_RECOMMENDER_H
