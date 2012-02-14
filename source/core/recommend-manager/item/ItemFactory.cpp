#include "ItemFactory.h"
#include "LocalItemIdGenerator.h"
#include "LocalItemManager.h"

namespace sf1r
{

ItemFactory::ItemFactory(IDManager* idManager, DocumentManager* docManager)
    : idManager_(idManager)
    , docManager_(docManager)
    , itemIdGenerator_(NULL)
    , itemManager_(NULL)
{
}

ItemIdGenerator* ItemFactory::createItemIdGenerator()
{
    if (! itemIdGenerator_)
    {
        itemIdGenerator_ = new LocalItemIdGenerator(*idManager_, *docManager_);
    }

    return itemIdGenerator_;
}

ItemManager* ItemFactory::createItemManager()
{
    if (! itemManager_)
    {
        itemManager_ = new LocalItemManager(*docManager_);
    }

    return itemManager_;
}

} // namespace sf1r
