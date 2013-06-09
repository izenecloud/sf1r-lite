#include "CategoryScoreEvaluator.h"

using namespace sf1r;

namespace
{
/*
 * how many results are displayed in search result page,
 * used to limit the diversity range for long query.
 */
const int kPageResultNum = 12;
}

CategoryScoreEvaluator::CategoryScoreEvaluator(score_t weight, bool isLongQuery)
    : ProductScoreEvaluator("category")
    , weight_(weight)
    , isLongQuery_(isLongQuery)
    , longQueryResultCount_(0)
{
}

score_t CategoryScoreEvaluator::evaluate(ProductScore& productScore)
{
    if (weight_ == 0)
        return 0;

    score_t result = static_cast<int>(productScore.topKScore_ / weight_);

    if (isLongQuery_ && result == 0)
    {
        result = longQueryResultCount_-- / kPageResultNum;
    }

    return result;
}
