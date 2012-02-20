#include "ItemFactory.h"
#include "LocalItemIdGenerator.h"
#include "LocalItemManager.h"

namespace sf1r
{

ItemFactory::ItemFactory(IDManager* idManager, DocumentManager* docManager)
    : idManager_(idManager)
    , docManager_(docManager)
{
}

ItemIdGenerator* ItemFactory::createItemIdGenerator()
{
    return new LocalItemIdGenerator(*idManager_);
}

ItemManager* ItemFactory::createItemManager()
{
    return new LocalItemManager(*docManager_);
}

} // namespace sf1r
