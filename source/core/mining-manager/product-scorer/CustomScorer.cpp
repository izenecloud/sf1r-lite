#include "CustomScorer.h"

using namespace sf1r;

namespace
{
/**
 * in order to give the highest priority to custom score, as we assume
 * the sum of other scores (multiplied by their weight) are less than 10,
 * we choose 10 as its weight accordingly.
 */
const score_t kCustomScoreWeight = 10;
}

CustomScorer::CustomScorer(const std::vector<docid_t>& topIds)
    : ProductScorer(kCustomScoreWeight)
{
    score_t score = 1;

    for (std::vector<docid_t>::const_reverse_iterator rit = topIds.rbegin();
         rit != topIds.rend(); ++rit)
    {
        scoreMap_[*rit] = score;
        ++score;
    }
}

score_t CustomScorer::score(docid_t docId)
{
    ScoreMap::const_iterator it = scoreMap_.find(docId);

    if (it == scoreMap_.end())
        return 0;

    return it->second;
}
