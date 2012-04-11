/**
 * @file PurchaseMatrix.h
 * @brief update item CF matrix of purchase items
 * @author Jun Jiang
 * @date 2012-01-19
 */

#ifndef PURCHASE_MATRIX_H
#define PURCHASE_MATRIX_H

#include "RecommendMatrix.h"
#include <aggregator-manager/UpdateRecommendBase.h>

namespace sf1r
{

class PurchaseMatrix : public RecommendMatrix
{
public:
    PurchaseMatrix(UpdateRecommendBase& matrix)
        : matrix_(matrix)
    {}

    virtual void update(
        const std::list<itemid_t>& oldItems,
        const std::list<itemid_t>& newItems
    )
    {
        bool result = true;
        matrix_.updatePurchaseMatrix(oldItems, newItems, result);
    }

private:
    UpdateRecommendBase& matrix_;
};

} // namespace sf1r

#endif // PURCHASE_MATRIX_H
