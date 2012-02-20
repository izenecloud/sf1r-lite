/**
 * @file PurchaseCoVisitMatrix.h
 * @brief update covist matrix of purchase items
 * @author Jun Jiang
 * @date 2012-01-19
 */

#ifndef PURCHASE_COVISIT_MATRIX_H
#define PURCHASE_COVISIT_MATRIX_H

#include "RecommendMatrix.h"

namespace sf1r
{

class PurchaseCoVisitMatrix : public RecommendMatrix
{
public:
    PurchaseCoVisitMatrix(ItemCFManager& matrix)
        : matrix_(matrix)
    {}

    virtual void update(
        const std::list<itemid_t>& oldItems,
        const std::list<itemid_t>& newItems
    )
    {
        matrix_.updateVisitMatrix(oldItems, newItems);
    }

private:
    ItemCFManager& matrix_;
};

} // namespace sf1r

#endif //PURCHASE_COVISIT_MATRIX_H
