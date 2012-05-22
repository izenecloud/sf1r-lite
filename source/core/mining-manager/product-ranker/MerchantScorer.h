/**
 * @file MerchantScorer.h
 * @brief get two scores:
 * 1. merchant count: 0 for no merchant, 1 for one merchant, 2 for multiple merchants;
 * 2. merchant score: the sum of merchant scores.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-05-17
 */

#ifndef SF1R_MERCHANT_SCORER_H
#define SF1R_MERCHANT_SCORER_H

#include "ProductScorer.h"

namespace sf1r
{
class MerchantScoreManager;

namespace faceted
{
class PropValueTable;
}

class MerchantScorer : public ProductScorer
{
public:
    MerchantScorer(
        const faceted::PropValueTable* categoryValueTable,
        const faceted::PropValueTable* merchantValueTable,
        const MerchantScoreManager* merchantScoreManager
    );

    virtual void pushScore(
        const ProductRankingParam& param,
        ProductScoreMatrix& scoreMatrix
    );

private:
    const faceted::PropValueTable* categoryValueTable_;
    const faceted::PropValueTable* merchantValueTable_;

    const MerchantScoreManager* merchantScoreManager_;
};

} // namespace sf1r

#endif // SF1R_MERCHANT_SCORER_H
