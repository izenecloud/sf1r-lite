/**
 * @file ProductScorer.h
 * @brief the interface to calculate score for each product.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-05-17
 */

#ifndef SF1R_PRODUCT_SCORER_H
#define SF1R_PRODUCT_SCORER_H

#include "ProductRankingParam.h"
#include "ProductScoreList.h"

namespace sf1r
{

class ProductScorer
{
public:
    virtual ~ProductScorer() {}

    virtual void pushScore(
        const ProductRankingParam& param,
        ProductScoreMatrix& scoreMatrix
    ) = 0;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORER_H
