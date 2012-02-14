#include "BORRecommender.h"
#include "ItemFilter.h"
#include "UserEventFilter.h"
#include "../item/ItemManager.h"
#include "../common/RecommendParam.h"
#include "../common/RecommendItem.h"

#include <glog/logging.h>

#include <ctime>

namespace sf1r
{

BORRecommender::BORRecommender(
    ItemManager& itemManager,
    const UserEventFilter& userEventFilter
)
    : Recommender(itemManager)
    , userEventFilter_(userEventFilter)
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

    if (!param.userIdStr.empty() &&
        !userEventFilter_.filter(param.userIdStr, param.inputItemIds, filter))
    {
        LOG(ERROR) << "failed to filter user event for user id " << param.userIdStr;
        return false;
    }

    const itemid_t minItemId = 1;
    const itemid_t maxItemId = itemManager_.maxItemId();
    if (minItemId > maxItemId)
        return true;

    RecommendItem recItem;
    recItem.weight_ = 1;

    const int maxInsertNum = param.limit;
    const int maxTryNum = param.limit << 1;
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
