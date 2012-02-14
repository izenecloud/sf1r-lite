#include "BABRecommender.h"
#include "../common/RecommendParam.h"

#include <glog/logging.h>

namespace sf1r
{

BABRecommender::BABRecommender(
    ItemManager& itemManager,
    ItemCFManager& itemCFManager
)
    : ItemCFRecommender(itemManager, itemCFManager)
{
}

bool BABRecommender::recommendImpl_(
    RecommendParam& param,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    if (param.inputItemIds.empty())
    {
        LOG(ERROR) << "failed to recommend for empty input items";
        return false;
    }

    if (! ItemCFRecommender::recommendImpl_(param, filter, recItemVec))
        return false;

    return true;
}

} // namespace sf1r
