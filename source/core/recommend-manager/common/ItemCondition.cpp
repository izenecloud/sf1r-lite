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
    if (filteringList_.empty() || !queryBuilder)
        return;

    boost::shared_ptr<InvertedIndexManager::FilterBitmapT> filterBitmap;
    queryBuilder->prepare_filter(filteringList_, filterBitmap);

    pBitset_.reset(new izenelib::ir::indexmanager::Bitset);
    pBitset_->importFromEWAH(*filterBitmap);
}

bool ItemCondition::isMeetCondition(itemid_t itemId) const
{
    if (pBitset_)
        return pBitset_->test(itemId);

    if (itemManager_)
        return itemManager_->hasItem(itemId);

    return false;
}

} // namespace sf1r
