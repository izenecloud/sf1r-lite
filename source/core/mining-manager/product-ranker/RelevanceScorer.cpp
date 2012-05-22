#include "RelevanceScorer.h"

namespace sf1r
{

void RelevanceScorer::pushScore(
    const ProductRankingParam& param,
    ProductScoreMatrix& scoreMatrix
)
{
    for (ProductScoreMatrix::iterator it = scoreMatrix.begin();
        it != scoreMatrix.end(); ++it)
    {
        it->pushScore(it->relevanceScore_);
    }
}

} // namespace sf1r
