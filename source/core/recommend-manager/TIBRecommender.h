/**
 * @file TIBRecommender.h
 * @brief recommend "top item bundle"
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef TIB_RECOMMENDER_H
#define TIB_RECOMMENDER_H

#include "RecTypes.h"

#include <vector>

namespace sf1r
{
class OrderManager;

class TIBRecommender
{
public:
    TIBRecommender(OrderManager& orderManager);

    bool recommend(
        int maxRecNum,
        int minFreq,
        std::vector<vector<itemid_t> >& bundleVec,
        std::vector<int>& freqVec
    );

private:
    OrderManager& orderManager_;
};

} // namespace sf1r

#endif // TIB_RECOMMENDER_H
