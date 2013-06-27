#include "ItemCondition.h"
#include "../item/ItemManager.h"
#include <search-manager/QueryBuilder.h>

namespace sf1r
{

ItemCondition::ItemCondition()
    : itemManager_(NULL)
{
}

void ItemCondition::createBitVector(QueryBuilder* queryBuilder)
{
    // no condition
    if (filteringList_.empty() || !queryBuilder)
        return;

    boost::shared_ptr<InvertedIndexManager::FilterBitmapT> filterBitmap;
    queryBuilder->prepare_filter(filteringList_, filterBitmap);

    pBitVector_.reset(new izenelib::ir::indexmanager::BitVector);
    pBitVector_->importFromEWAH(*filterBitmap);
}

bool ItemCondition::isMeetCondition(itemid_t itemId) const
{
    if (pBitVector_)
        return pBitVector_->test(itemId);

    if (itemManager_)
        return itemManager_->hasItem(itemId);

    return false;
}

} // namespace sf1r
