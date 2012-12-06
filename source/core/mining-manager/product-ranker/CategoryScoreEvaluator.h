/**
 * @file CategoryScoreEvaluator.h
 * @brief extract the category score from topK score.
 *
 * The calculation function is:
 * category score = (int)(topK score / category weight)
 *
 * For example, given the topK score is 123.45,
 * if the category weight is 1, then the "category score" is 123,
 * while if the category weight is 10, then the "category score" is 12.
 *
 * @author Jun Jiang
 * @date Created 2012-12-03
 */

#ifndef SF1R_CATEGORY_SCORE_EVALUATOR_H
#define SF1R_CATEGORY_SCORE_EVALUATOR_H

#include "ProductScoreEvaluator.h"

namespace sf1r
{
class ProductRankingConfig;

class CategoryScoreEvaluator : public ProductScoreEvaluator
{
public:
    CategoryScoreEvaluator(const ProductRankingConfig& config);

    virtual score_t evaluate(ProductScore& productScore);

private:
    const score_t weight_;
};

} // namespace sf1r

#endif // SF1R_CATEGORY_SCORE_EVALUATOR_H
