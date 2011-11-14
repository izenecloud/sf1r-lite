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
        bundle.freq = results[i].second;

        const std::vector<itemid_t>& itemIds = results[i].first;
        for (vector<itemid_t>::const_iterator it = itemIds.begin();
            it != itemIds.end(); ++it)
        {
            bundle.items.push_back(Document(*it));
        }
    }

    return true;
}

} // namespace sf1r
