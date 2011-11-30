#include "BORRecommender.h"
#include "ItemManager.h"
#include "ItemFilter.h"
#include "RecommendParam.h"
#include "RecommendItem.h"

#include <glog/logging.h>

#include <ctime>

namespace sf1r
{

BORRecommender::BORRecommender(ItemManager& itemManager)
    : Recommender(itemManager)
{
    rand_.seed(std::time(NULL));
}

bool BORRecommender::recommendImpl_(
    RecommendParam& param,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    const std::vector<itemid_t>& inputItemIds = param.inputItemIds;
    filter.insert(inputItemIds.begin(), inputItemIds.end());

    RecommendItem recItem;
    recItem.weight_ = 1;

    const int maxInsertNum = param.limit;
    const int maxTryNum = param.limit << 1;
    int insertNum = 0;
    int tryNum = 0;

    const itemid_t minItemId = 1;
    const itemid_t maxItemId = itemManager_.maxItemId();

    while (insertNum < maxInsertNum
           && tryNum < maxTryNum)
    {
        itemid_t id = rand_.generate(minItemId, maxItemId);
        if (! filter.isFiltered(id))
        {
            recItem.item_.setId(id);
            recItemVec.push_back(recItem);

            filter.insert(id);
            ++insertNum;
        }
        ++tryNum;
    }

    return true;
}

} // namespace sf1r
