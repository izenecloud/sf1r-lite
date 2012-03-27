#include "GetRecommendAggregator.h"
#include <recommend-manager/common/ItemFilter.h>

namespace sf1r
{

GetRecommendAggregator::GetRecommendAggregator()
{
}

bool GetRecommendAggregator::recommendPurchase(
    RecommendInputParam& inputParam,
    idmlib::recommender::RecommendItemVec& results
)
{
    return true;
}

bool GetRecommendAggregator::recommendPurchaseFromWeight(
    RecommendInputParam& inputParam,
    idmlib::recommender::RecommendItemVec& results
)
{
    return true;
}

bool GetRecommendAggregator::recommendVisit(
    RecommendInputParam& inputParam,
    std::vector<itemid_t>& results
)
{
    return true;
}

} // namespace sf1r
