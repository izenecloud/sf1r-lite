#include "VAVRecommender.h"
#include "ItemFilter.h"

#include <glog/logging.h>

namespace sf1r
{

VAVRecommender::VAVRecommender(CoVisitManager* coVisitManager)
    : coVisitManager_(coVisitManager)
{
}

bool VAVRecommender::recommend(
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

    std::vector<itemid_t> results;
    coVisitManager_->getCoVisitation(maxRecNum, inputItemVec[0], results, &filter);

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
