/**
 * @file UpdateRecommendBase.h
 * @brief base class to update recommendation result
 * @author Jun Jiang
 * @date 2012-03-26
 */

#ifndef SF1R_UPDATE_RECOMMEND_BASE_H
#define SF1R_UPDATE_RECOMMEND_BASE_H

#include <recommend-manager/common/RecTypes.h>
#include <list>

namespace sf1r
{

class UpdateRecommendBase
{
public:
    virtual ~UpdateRecommendBase() {}

    virtual bool updatePurchaseMatrix(
        const std::list<itemid_t>& oldItems,
        const std::list<itemid_t>& newItems
    ) = 0;

    virtual bool updatePurchaseCoVisitMatrix(
        const std::list<itemid_t>& oldItems,
        const std::list<itemid_t>& newItems
    ) = 0;

    virtual bool buildPurchaseSimMatrix() = 0;

    virtual bool updateVisitMatrix(
        const std::list<itemid_t>& oldItems,
        const std::list<itemid_t>& newItems
    ) = 0;

    virtual bool flushRecommendMatrix() = 0;
};

} // namespace sf1r

#endif // SF1R_UPDATE_RECOMMEND_BASE_H
