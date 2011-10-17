#include "TIBRecommender.h"
#include "OrderManager.h"
#include "TIBParam.h"
#include "ItemBundle.h"

namespace sf1r
{

TIBRecommender::TIBRecommender(OrderManager& orderManager)
    : orderManager_(orderManager)
{
}

bool TIBRecommender::recommend(
    const TIBParam& param,
    std::vector<ItemBundle>& bundleVec
)
{
    FrequentItemSetResultType results;
    orderManager_.getAllFreqItemSets(param.limit, param.minFreq,
                                     results);

    const std::size_t resultNum = results.size();
    bundleVec.resize(resultNum);
    for (std::size_t i = 0; i < resultNum; ++i)
    {
        ItemBundle& bundle = bundleVec[i];
        bundle.itemIds.swap(results[i].first);
        bundle.freq = results[i].second;
    }

    return true;
}

} // namespace sf1r
