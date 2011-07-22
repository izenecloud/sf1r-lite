#include "AggregatorManager.h"

namespace sf1r
{

void AggregatorManager::mergeSearchResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList)
{
    if (resultList.size() == 1)
        result = resultList[0].second;

    for (size_t i = 0; i < resultList.size(); i++)
    {
    }
}

void AggregatorManager::mergeSummaryResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList)
{
    // todo
}

}
