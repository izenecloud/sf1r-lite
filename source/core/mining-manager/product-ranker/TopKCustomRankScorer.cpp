#include "TopKCustomRankScorer.h"

namespace
{
/**
 * all custom scores should be larger than this value.
 */
sf1r::score_t CUSTOM_RANK_BASE_SCORE = 100;
}

namespace sf1r
{

TopKCustomRankScorer::TopKCustomRankScorer()
    : ProductScorer("custom")
{
}

void TopKCustomRankScorer::pushScore(
    const ProductRankingParam& param,
    ProductScoreMatrix& scoreMatrix
)
{
    for (ProductScoreMatrix::iterator it = scoreMatrix.begin();
        it != scoreMatrix.end(); ++it)
    {
        score_t score = 0;

        if (it->relevanceScore_ > CUSTOM_RANK_BASE_SCORE)
        {
            score = it->relevanceScore_;
        }

        it->pushScore(score);
    }
}

} // namespace sf1r
