#include "LocalItemFactory.h"
#include "LocalItemIdGenerator.h"
#include "LocalItemManager.h"

namespace sf1r
{

LocalItemFactory::LocalItemFactory(IDManager& idManager, DocumentManager& docManager)
    : idManager_(idManager)
    , docManager_(docManager)
{
}

ItemIdGenerator* LocalItemFactory::createItemIdGenerator()
{
    return new LocalItemIdGenerator(idManager_);
}

ItemManager* LocalItemFactory::createItemManager()
{
    return new LocalItemManager(docManager_);
}

} // namespace sf1r
