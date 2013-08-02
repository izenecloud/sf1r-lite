#include "CategoryScoreEvaluator.h"

using namespace sf1r;

namespace
{
/*
 * how many results are displayed in search result page,
 * used to limit the diversity range.
 */
const int kEachPageResultNum = 12;
}

CategoryScoreEvaluator::CategoryScoreEvaluator(score_t weight, bool isDiverseInPage)
    : ProductScoreEvaluator("category")
    , weight_(weight)
    , isDiverseInPage_(isDiverseInPage)
    , resultCount_(0)
{
}

score_t CategoryScoreEvaluator::evaluate(ProductScore& productScore)
{
    if (weight_ == 0)
        return 0;

    score_t result = static_cast<int>(productScore.topKScore_ / weight_);

    if (isDiverseInPage_ && result == 0)
    {
        result = resultCount_-- / kEachPageResultNum;
    }

    return result;
}
