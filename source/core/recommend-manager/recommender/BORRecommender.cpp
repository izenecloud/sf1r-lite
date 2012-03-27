#include "BORRecommender.h"
#include "UserEventFilter.h"
#include "../item/ItemIdGenerator.h"

#include <glog/logging.h>

#include <ctime>

namespace sf1r
{

BORRecommender::BORRecommender(
    const UserEventFilter& userEventFilter,
    const ItemIdGenerator& itemIdGenerator
)
    : userEventFilter_(userEventFilter)
    , itemIdGenerator_(itemIdGenerator)
{
    rand_.seed(std::time(NULL));
}

bool BORRecommender::recommendImpl_(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    std::vector<itemid_t>& inputItemIds = param.inputParam.inputItemIds;
    ItemFilter& filter = param.inputParam.itemFilter;
    filter.insert(inputItemIds.begin(), inputItemIds.end());

    if (!param.userIdStr.empty() &&
        !userEventFilter_.filter(param.userIdStr, inputItemIds, filter))
    {
        LOG(ERROR) << "failed to filter user event for user id " << param.userIdStr;
        return false;
    }

    const itemid_t minItemId = 1;
    const itemid_t maxItemId = itemIdGenerator_.maxItemId();
    if (minItemId > maxItemId)
        return true;

    RecommendItem recItem;
    recItem.weight_ = 1;

    const int maxInsertNum = param.inputParam.limit;
    const int maxTryNum = maxInsertNum << 1;
    int insertNum = 0;
    int tryNum = 0;

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
