/**
 * @file BAQRecommender.h
 * @brief recommend items "bought after query"
 * @author Jun Jiang
 * @date 2012-03-16
 */

#ifndef BAQ_RECOMMENDER_H
#define BAQ_RECOMMENDER_H

#include "Recommender.h"
#include "../common/RecTypes.h"

namespace sf1r
{

class BAQRecommender : public Recommender
{
public:
    BAQRecommender(
        ItemManager& itemManager,
        QueryClickCounter& queryPurchaseCounter
    );

protected:
    virtual bool recommendImpl_(
        RecommendParam& param,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

private:
    QueryClickCounter& queryPurchaseCounter_;
};

} // namespace sf1r

#endif // BAQ_RECOMMENDER_H
