#include "TIBRecommender.h"
#include "OrderManager.h"

namespace sf1r
{

TIBRecommender::TIBRecommender(OrderManager* orderManager)
    : orderManager_(orderManager)
{
}

bool TIBRecommender::recommend(
    int maxRecNum,
    int minFreq,
    std::vector<vector<itemid_t> >& bundleVec,
    std::vector<int>& freqVec
)
{
    FrequentItemSetResultType results;
    orderManager_->getAllFreqItemSets(maxRecNum, minFreq, results);

    const std::size_t resultNum = results.size();
    bundleVec.resize(resultNum);
    freqVec.resize(resultNum);
    for (std::size_t i = 0; i < resultNum; ++i)
    {
        bundleVec[i].swap(results[i].first);
        freqVec[i] = results[i].second;
    }

    return true;
}

} // namespace sf1r
