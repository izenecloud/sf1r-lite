#include "CustomRankScorer.h"
#include <algorithm>

namespace sf1r
{

CustomRankScorer::CustomRankScorer(const CustomRankValue& customValue)
    : sortCustomValue_(customValue)
{
    const CustomRankValue::DocIdList& topIds = customValue.topIds;
    std::size_t relativeScore = topIds.size();

    for (CustomRankValue::DocIdList::const_iterator it = topIds.begin();
        it != topIds.end(); ++it)
    {
        scoreMap_[*it] = CUSTOM_RANK_BASE_SCORE + relativeScore;
        --relativeScore;
    }

    std::sort(sortCustomValue_.topIds.begin(), sortCustomValue_.topIds.end());
    std::sort(sortCustomValue_.excludeIds.begin(), sortCustomValue_.excludeIds.end());
}

} // namespace sf1r
