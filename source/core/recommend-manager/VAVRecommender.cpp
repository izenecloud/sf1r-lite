#include "VAVRecommender.h"
#include "ItemFilter.h"
#include "RecommendParam.h"
#include "RecommendItem.h"

#include <glog/logging.h>

namespace sf1r
{

VAVRecommender::VAVRecommender(
    ItemManager& itemManager,
    CoVisitManager& coVisitManager
)
    : Recommender(itemManager)
    , coVisitManager_(coVisitManager)
{
}

bool VAVRecommender::recommendImpl_(
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

    std::vector<itemid_t> results;
    coVisitManager_.getCoVisitation(param.limit, param.inputItemIds[0], results, &filter);

    RecommendItem recItem;
    recItem.weight_ = 1;
    for (std::vector<itemid_t>::const_iterator it = results.begin();
        it != results.end(); ++it)
    {
        recItem.itemId_ = *it;
        recItemVec.push_back(recItem);
    }

    return true;
}

} // namespace sf1r
