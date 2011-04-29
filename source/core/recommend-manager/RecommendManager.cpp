#include "RecommendManager.h"

#include <glog/logging.h>

namespace sf1r
{

bool RecommendManager::recommend(
    RecommendType type,
    int maxRecNum,
    userid_t userId,
    const std::vector<itemid_t>& inputItemVec,
    const std::vector<itemid_t>& includeItemVec,
    const std::vector<itemid_t>& excludeItemVec,
    /*const std::vector<ItemCategory>& filterCategoryVec,*/
    std::vector<itemid_t>& recItemVec,
    std::vector<double>& recWeightVec
)
{
    return true;
}

} // namespace sf1r
