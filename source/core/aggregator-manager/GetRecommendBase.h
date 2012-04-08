/**
 * @file GetRecommendBase.h
 * @brief base class to get recommendation result
 * @author Jun Jiang
 * @date 2012-03-26
 */

#ifndef SF1R_GET_RECOMMEND_BASE_H
#define SF1R_GET_RECOMMEND_BASE_H

#include <recommend-manager/common/RecommendInputParam.h>
#include <vector>

namespace sf1r
{

class GetRecommendBase
{
public:
    virtual ~GetRecommendBase() {}

    virtual void recommendPurchase(
        const RecommendInputParam& inputParam,
        idmlib::recommender::RecommendItemVec& results
    ) = 0;

    virtual void recommendPurchaseFromWeight(
        const RecommendInputParam& inputParam,
        idmlib::recommender::RecommendItemVec& results
    ) = 0;

    virtual void recommendVisit(
        const RecommendInputParam& inputParam,
        std::vector<itemid_t>& results
    ) = 0;
};

} // namespace sf1r

#endif // SF1R_GET_RECOMMEND_BASE_H
