#include "ZambeziScoreNormalizer.h"
#include <mining-manager/MiningManager.h>
#include <mining-manager/attr-manager/AttrTable.h>
#include <util/fmath/fmath.hpp>

namespace sf1r
{

ZambeziScoreNormalizer::ZambeziScoreNormalizer(MiningManager& miningManager)
        : exponentScorer_(miningManager.getMiningSchema().
                          product_ranking_config.scores[ZAMBEZI_RELEVANCE_SCORE])
        , relevanceWeight_(exponentScorer_.weight())
        , attrTable_(miningManager.GetAttrTable())
{
}

void ZambeziScoreNormalizer::normalizeScore(
    const std::vector<docid_t>& docids,
    const std::vector<float>& productScores,
    std::vector<float>& relevanceScores,
    PropSharedLockSet& sharedLockSet)
{
    if (attrTable_)
    {
        sharedLockSet.insertSharedLock(attrTable_);
    }

    for (uint32_t i = 0; i < docids.size(); ++i)
    {
        float attrScore = 1;
        if (attrTable_)
        {
            faceted::AttrTable::ValueIdList vids;
            attrTable_->getValueIdList(docids[i], vids);
            attrScore += std::min(vids.size(), std::size_t(30)) * 10;
        }

        float normScore = relevanceScores[i] + attrScore;
        normScore = exponentScorer_.calculate(normScore);
        relevanceScores[i] = normScore*relevanceWeight_ + productScores[i];
    }
}

}
