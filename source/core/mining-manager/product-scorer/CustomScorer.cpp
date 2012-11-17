#include "CustomScorer.h"

using namespace sf1r;

CustomScorer::CustomScorer(
    const ProductScoreConfig& config,
    const std::vector<docid_t>& topIds)
    : ProductScorer(config)
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
