#include "RemoteItemFactory.h"
#include "RemoteItemIdGenerator.h"
#include "RemoteItemManager.h"

namespace sf1r
{

RemoteItemFactory::RemoteItemFactory(const std::string& collection)
    : collection_(collection)
{
}

ItemIdGenerator* RemoteItemFactory::createItemIdGenerator()
{
    return new RemoteItemIdGenerator(collection_);
}

ItemManager* RemoteItemFactory::createItemManager()
{
    return new RemoteItemManager(collection_, createItemIdGenerator());
}

} // namespace sf1r
