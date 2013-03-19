/**
 * @file MerchantScoreEvaluator.h
 * @brief get merchant score.
 *
 * given a docid, if it has multiple merchants, then its merchant score is zero;
 * otherwise, get its merchant score by calling MerchantScoreManager::getIdScore().
 *
 * @author Jun Jiang
 * @date Created 2013-01-24
 */

#ifndef SF1R_MERCHANT_SCORE_EVALUATOR_H
#define SF1R_MERCHANT_SCORE_EVALUATOR_H

#include "ProductScoreEvaluator.h"
#include <common/PropSharedLock.h>
#include <vector>

namespace sf1r
{
class MerchantScoreManager;
namespace faceted { class PropValueTable; }

class MerchantScoreEvaluator : public ProductScoreEvaluator
{
public:
    MerchantScoreEvaluator(
        const MerchantScoreManager& merchantScoreManager);

    MerchantScoreEvaluator(
        const MerchantScoreManager& merchantScoreManager,
        const faceted::PropValueTable& categoryValueTable);

    virtual score_t evaluate(ProductScore& productScore);

private:
    const MerchantScoreManager& merchantScoreManager_;

    const faceted::PropValueTable* categoryValueTable_;

    PropSharedLock::ScopedReadLock lock_;

    std::vector<category_id_t> parentIds_;
};

} // namespace sf1r

#endif // SF1R_MERCHANT_SCORE_EVALUATOR_H
