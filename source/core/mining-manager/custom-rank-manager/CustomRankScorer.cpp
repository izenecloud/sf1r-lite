#include "CustomRankScorer.h"

namespace sf1r
{

CustomRankScorer::CustomRankScorer(const std::vector<docid_t>& docIdList)
{
    std::size_t relativeScore = docIdList.size();

    for (std::vector<docid_t>::const_iterator it = docIdList.begin();
        it != docIdList.end(); ++it)
    {
        scoreMap_[*it] = CUSTOM_RANK_BASE_SCORE + relativeScore;
        --relativeScore;
    }
}

} // namespace sf1r
