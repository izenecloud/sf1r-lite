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

    virtual bool isMasterNode() const = 0;

    virtual void updatePurchaseMatrix(
        const std::list<itemid_t>& oldItems,
        const std::list<itemid_t>& newItems,
        bool& result
    ) = 0;

    virtual void updatePurchaseCoVisitMatrix(
        const std::list<itemid_t>& oldItems,
        const std::list<itemid_t>& newItems,
        bool& result
    ) = 0;

    virtual void buildPurchaseSimMatrix(bool& result) = 0;

    virtual void updateVisitMatrix(
        const std::list<itemid_t>& oldItems,
        const std::list<itemid_t>& newItems,
        bool& result
    ) = 0;

    virtual void flushRecommendMatrix(bool& result) = 0;
};

} // namespace sf1r

#endif // SF1R_UPDATE_RECOMMEND_BASE_H
