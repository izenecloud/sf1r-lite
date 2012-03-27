#include "BABRecommender.h"

#include <glog/logging.h>

namespace sf1r
{

BABRecommender::BABRecommender(ItemCFManager& itemCFManager)
    : ItemCFRecommender(itemCFManager)
{
}

bool BABRecommender::recommendImpl_(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    if (param.inputParam.inputItemIds.empty())
    {
        LOG(ERROR) << "failed to recommend for empty input items";
        return false;
    }

    if (! ItemCFRecommender::recommendImpl_(param, recItemVec))
        return false;

    return true;
}

} // namespace sf1r
