#include "Recommender.h"
#include "ItemFilter.h"
#include "../item/ItemManager.h"
#include "../common/RecommendParam.h"
#include "../common/RecommendItem.h"

#include <algorithm> // std::min

namespace
{
using namespace sf1r;

void copyIncludeItems(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    recItemVec.clear();

    const std::size_t includeNum = std::min(param.includeItemIds.size(),
                                            static_cast<std::size_t>(param.limit));
    RecommendItem recItem;
    recItem.weight_ = 1;

    for (std::size_t i = 0; i < includeNum; ++i)
    {
        recItem.item_.setId(param.includeItemIds[i]);
        recItemVec.push_back(recItem);
    }

    param.limit -= includeNum;
}

}

namespace sf1r
{

Recommender::Recommender(ItemManager& itemManager)
    : itemManager_(itemManager)
{
}

bool Recommender::recommend(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    if (param.limit <= 0)
        return false;

    copyIncludeItems(param, recItemVec);
    if (param.limit == 0)
        return true;

    ItemFilter filter(itemManager_, param);
    return recommendImpl_(param, filter, recItemVec);
}

} // namespace sf1r
