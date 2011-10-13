#include "BABRecommender.h"
#include "ItemFilter.h"

#include <glog/logging.h>

namespace sf1r
{

BABRecommender::BABRecommender(ItemCFManager* itemCFManager)
    : itemCFManager_(itemCFManager)
{
}

bool BABRecommender::recommend(
    int maxRecNum,
    const std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    if (inputItemVec.empty())
    {
        LOG(ERROR) << "failed to recommend for empty input items";
        return false;
    }

    idmlib::recommender::RecommendItemVec results;
    itemCFManager_->recommend(maxRecNum, inputItemVec, results, &filter);

    recItemVec.insert(recItemVec.end(), results.begin(), results.end());

    return true;
}

} // namespace sf1r
