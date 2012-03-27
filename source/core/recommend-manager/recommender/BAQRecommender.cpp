#include "BAQRecommender.h"

namespace sf1r
{

BAQRecommender::BAQRecommender(QueryClickCounter& queryPurchaseCounter)
    : queryPurchaseCounter_(queryPurchaseCounter)
{
}

bool BAQRecommender::recommendImpl_(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    typedef QueryClickCounter::click_counter_type ClickCounter;
    ClickCounter clickCounter;
    if (! queryPurchaseCounter_.get(param.query, clickCounter))
        return false;

    std::vector<itemid_t> itemIds;
    std::vector<ClickCounter::freq_type> freqs;
    clickCounter.getFreqClick(param.inputParam.limit, itemIds, freqs);

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

    param.queryClickFreq = clickCounter.getTotalFreq();

    return true;
}

} // namespace sf1r
