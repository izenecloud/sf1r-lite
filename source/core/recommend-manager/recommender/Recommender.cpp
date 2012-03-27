#include "Recommender.h"

#include <algorithm> // std::min

namespace
{
using namespace sf1r;

void copyIncludeItems(
    const std::vector<itemid_t>& includeItemIds,
    int& limitNum,
    std::vector<RecommendItem>& recItemVec
)
{
    recItemVec.clear();

    const std::size_t includeNum = std::min(includeItemIds.size(),
                                            static_cast<std::size_t>(limitNum));
    RecommendItem recItem;
    recItem.weight_ = 1;

    for (std::size_t i = 0; i < includeNum; ++i)
    {
        recItem.item_.setId(includeItemIds[i]);
        recItemVec.push_back(recItem);
    }

    limitNum -= includeNum;
}

}

namespace sf1r
{

bool Recommender::recommend(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    int& limit = param.inputParam.limit;
    if (limit <= 0)
        return false;

    copyIncludeItems(param.includeItemIds, limit, recItemVec);
    if (limit == 0)
        return true;

    ItemFilter& filter = param.inputParam.itemFilter;
    filter.insert(param.includeItemIds.begin(), param.includeItemIds.end());
    filter.insert(param.excludeItemIds.begin(), param.excludeItemIds.end());

    return recommendImpl_(param, recItemVec);
}

} // namespace sf1r
