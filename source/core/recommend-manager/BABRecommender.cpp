#include "BABRecommender.h"
#include "ItemFilter.h"

#include <glog/logging.h>

namespace sf1r
{

BABRecommender::BABRecommender(ItemManager& itemManager, ItemCFManager& itemCFManager)
    : Recommender(itemManager)
    , itemCFManager_(itemCFManager)
{
}

bool BABRecommender::recommend(RecommendParam& param, std::vector<RecommendItem>& recItemVec)
{
    if (param.inputItemIds.empty())
    {
        LOG(ERROR) << "failed to recommend for empty input items";
        return false;
    }

    ItemFilter filter(itemManager_, param);

    idmlib::recommender::RecommendItemVec results;
    itemCFManager_.recommend(param.limit, param.inputItemIds, results, &filter);

    recItemVec.insert(recItemVec.end(), results.begin(), results.end());

    return true;
}

} // namespace sf1r
