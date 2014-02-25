#include "ItemCondition.h"
#include "../item/ItemManager.h"
#include <search-manager/QueryBuilder.h>

namespace sf1r
{

ItemCondition::ItemCondition()
    : itemManager_(NULL)
{
}

void ItemCondition::createBitset(QueryBuilder* queryBuilder)
{
    // no condition
    if (filterTree_.empty() || !queryBuilder)
        return;

    queryBuilder->prepare_filter(filterTree_, pFilterBitmap_);
}

bool ItemCondition::isMeetCondition(itemid_t itemId) const
{
    if (pFilterBitmap_)
        return pFilterBitmap_->test(itemId);

    if (itemManager_)
        return itemManager_->hasItem(itemId);

    return false;
}

} // namespace sf1r
