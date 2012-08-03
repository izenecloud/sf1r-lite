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

    boost::shared_ptr<izenelib::am::EWAHBoolArray<uint32_t> > ewahArray;
    queryBuilder->prepare_filter(filteringList_, ewahArray);

    pBitVector_.reset(new izenelib::ir::indexmanager::BitVector);
    pBitVector_->importFromEWAH(*ewahArray);
}

bool ItemCondition::isMeetCondition(itemid_t itemId) const
{
    if (! itemManager_)
        return false;

    // no condition
    if (filteringList_.empty())
        return itemManager_->hasItem(itemId);

    if (pBitVector_)
        return pBitVector_->test(itemId);

    return true;
}

} // namespace sf1r
