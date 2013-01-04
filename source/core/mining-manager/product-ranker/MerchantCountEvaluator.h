/**
 * @file MerchantCountEvaluator.h
 * @brief get merchant count as score.
 *
 * That is:
 * score 0 for no merchant;
 * score 1 for one merchant;
 * score 2 for multiple merchants;
 *
 * @author Jun Jiang
 * @date Created 2012-12-03
 */

#ifndef SF1R_MERCHANT_COUNT_EVALUATOR_H
#define SF1R_MERCHANT_COUNT_EVALUATOR_H

#include "ProductScoreEvaluator.h"
#include <common/PropSharedLock.h>

namespace sf1r
{
namespace faceted { class PropValueTable; }

class MerchantCountEvaluator : public ProductScoreEvaluator
{
public:
    MerchantCountEvaluator(const faceted::PropValueTable& merchantValueTable);

    virtual score_t evaluate(ProductScore& productScore);

private:
    const faceted::PropValueTable& merchantValueTable_;

    PropSharedLock::ScopedReadLock lock_;
};

} // namespace sf1r

#endif // SF1R_MERCHANT_COUNT_EVALUATOR_H
