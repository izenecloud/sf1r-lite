#include "ItemFilter.h"
#include "../item/ItemManager.h"
#include "../common/ItemCondition.h"
#include "../common/RecommendParam.h"
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

    const std::string& condPropName = condition_.propName_;

    // no condition
    if (condPropName.empty())
        return itemManager_.hasItem(itemId) == false;

    Document item;
    std::vector<std::string> propList;
    propList.push_back(condPropName);

    // not exist
    if (itemManager_.getItem(itemId, propList, item) == false)
        return true;

    return condition_.checkItem(item) == false;
}

} // namespace sf1r
