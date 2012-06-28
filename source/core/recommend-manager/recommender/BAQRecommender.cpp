#include "BAQRecommender.h"

namespace sf1r
{

BAQRecommender::BAQRecommender(QueryPurchaseCounter& queryPurchaseCounter)
    : queryPurchaseCounter_(queryPurchaseCounter)
{
}

bool BAQRecommender::recommendImpl_(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    PurchaseCounter purchaseCounter;
    if (! queryPurchaseCounter_.get(param.query, purchaseCounter))
        return false;

    std::vector<itemid_t> itemIds;
    std::vector<PurchaseCounter::freq_type> freqs;
    purchaseCounter.getFreqClick(param.inputParam.limit, itemIds, freqs);

    ItemFilter& filter = param.inputParam.itemFilter;
    RecommendItem recItem;
    const int itemNum = itemIds.size();

    for (int i=0; i<itemNum; ++i)
    {
        itemid_t itemId = itemIds[i];

        if (! filter.isFiltered(itemId))
        {
            recItem.item_.setId(itemId);
            recItem.weight_ = freqs[i];

            recItemVec.push_back(recItem);
        }
    }

    param.queryClickFreq = purchaseCounter.getTotalFreq();

    return true;
}

} // namespace sf1r
