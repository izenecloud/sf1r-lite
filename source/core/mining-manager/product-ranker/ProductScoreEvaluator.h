/**
 * @file ProductScoreEvaluator.h
 * @brief the interface to evaluate score for each product.
 * @author Jun Jiang
 * @date Created 2012-12-03
 */

#ifndef SF1R_PRODUCT_SCORE_EVALUATOR_H
#define SF1R_PRODUCT_SCORE_EVALUATOR_H

#include "ProductScore.h"
#include <string>

namespace sf1r
{

class ProductScoreEvaluator
{
public:
    ProductScoreEvaluator(const std::string& scoreName)
        : scoreName_(scoreName)
    {}

    virtual ~ProductScoreEvaluator() {}

    const std::string& getScoreName() const { return scoreName_; }

    virtual score_t evaluate(ProductScore& productScore) = 0;

private:
    std::string scoreName_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_EVALUATOR_H
