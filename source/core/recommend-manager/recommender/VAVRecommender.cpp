#include "VAVRecommender.h"

#include <glog/logging.h>

namespace sf1r
{

VAVRecommender::VAVRecommender(CoVisitManager& coVisitManager)
    : coVisitManager_(coVisitManager)
{
}

bool VAVRecommender::recommendImpl_(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    if (param.inputParam.inputItemIds.empty())
    {
        LOG(ERROR) << "failed to recommend for empty input items";
        return false;
    }

    std::vector<itemid_t> results;
    coVisitManager_.getCoVisitation(param.inputParam.limit, param.inputParam.inputItemIds[0],
                                    results, &param.inputParam.itemFilter);

    RecommendItem recItem;
    recItem.weight_ = 1;
    for (std::vector<itemid_t>::const_iterator it = results.begin();
        it != results.end(); ++it)
    {
        recItem.item_.setId(*it);
        recItemVec.push_back(recItem);
    }

    return true;
}

} // namespace sf1r
