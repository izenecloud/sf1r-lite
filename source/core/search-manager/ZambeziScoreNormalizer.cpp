#include "ZambeziScoreNormalizer.h"
#include <mining-manager/MiningManager.h>
#include <util/fmath/fmath.hpp>

namespace sf1r
{

ZambeziScoreNormalizer::ZambeziScoreNormalizer(MiningManager& miningManager)
        : exponentScorer_(miningManager.getMiningSchema().
                          product_ranking_config.scores[ZAMBEZI_RELEVANCE_SCORE])
        , relevanceWeight_(exponentScorer_.weight())
{
}

void ZambeziScoreNormalizer::normalizeScore(
    const std::vector<docid_t>& docids,
    const std::vector<float>& productScores,
    std::vector<float>& relevanceScores)
{
    for (uint32_t i = 0; i < docids.size(); ++i)
    {
        float normScore = exponentScorer_.calculate(relevanceScores[i]);
        relevanceScores[i] = normScore*relevanceWeight_ + productScores[i];
    }
}

}
