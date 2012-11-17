#include "PopularityScorer.h"
#include <cmath>

using namespace sf1r;

namespace
{

/**
 * @param x the input value should be >= 0
 * @return the normalized value in range [0, 1]
 */
score_t normalize(score_t x)
{
    return 1 - 1/log(x+3);
}

}

PopularityScorer::PopularityScorer(const ProductScoreConfig& config)
    : ProductScoreSum(config)
{
}

score_t PopularityScorer::score(docid_t docId)
{
    score_t sum = ProductScoreSum::score(docId);

    return normalize(sum);
}
