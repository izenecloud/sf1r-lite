#include "VAVRecommender.h"
#include <aggregator-manager/GetRecommendBase.h>

#include <glog/logging.h>

namespace sf1r
{

VAVRecommender::VAVRecommender(GetRecommendBase& getRecommendBase)
    : getRecommendBase_(getRecommendBase)
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
    getRecommendBase_.recommendVisit(param.inputParam, results);
    recItemVec.insert(recItemVec.end(), results.begin(), results.end());

    return true;
}

} // namespace sf1r
