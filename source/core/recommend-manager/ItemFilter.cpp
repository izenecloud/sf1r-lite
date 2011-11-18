#include "ItemFilter.h"
#include "ItemManager.h"
#include "ItemCondition.h"
#include "RecommendParam.h"
#include <document-manager/Document.h>

namespace sf1r
{

ItemFilter::ItemFilter(
    ItemManager& itemManager,
    const RecommendParam& param
)
    : itemManager_(itemManager)
    , condition_(param.condition)
{
    insert(param.includeItemIds.begin(), param.includeItemIds.end());
    insert(param.excludeItemIds.begin(), param.excludeItemIds.end());
}

bool ItemFilter::isFiltered(itemid_t itemId)
{
    // check filter set first
    if (filterSet_.find(itemId) != filterSet_.end())
        return true;

    // no condition
    if (condition_.propName_.empty())
        return itemManager_.hasItem(itemId) == false;

    // not exist
    Document item;
    if (itemManager_.getItem(itemId, item) == false)
        return true;

    return condition_.checkItem(item) == false;
}

} // namespace sf1r
