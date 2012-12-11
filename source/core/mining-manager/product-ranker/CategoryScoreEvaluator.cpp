#include "CategoryScoreEvaluator.h"

using namespace sf1r;

CategoryScoreEvaluator::CategoryScoreEvaluator(score_t weight)
    : ProductScoreEvaluator("category")
    , weight_(weight)
{
}

score_t CategoryScoreEvaluator::evaluate(ProductScore& productScore)
{
    if (weight_ == 0)
        return 0;

    score_t result = productScore.topKScore_ / weight_;
    return static_cast<int>(result);
}
