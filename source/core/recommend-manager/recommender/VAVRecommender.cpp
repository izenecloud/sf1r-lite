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

    idmlib::recommender::RecommendItemVec results;
    coVisitManager_.getCoVisitation(param.inputParam.limit, param.inputParam.inputItemIds[0],
                                    results, &param.inputParam.itemFilter);
    recItemVec.insert(recItemVec.end(), results.begin(), results.end());

    return true;
}

} // namespace sf1r
