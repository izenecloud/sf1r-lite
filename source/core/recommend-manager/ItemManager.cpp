#include "ItemManager.h"

namespace sf1r
{

ItemManager::ItemManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

void ItemManager::flush()
{
    container_.flush();
}

bool ItemManager::addItem(const Item& item)
{
    return container_.insertValue(item.id_, item);
}

bool ItemManager::updateItem(const Item& item)
{
    return container_.update(item.id_, item);
}

bool ItemManager::removeItem(itemid_t itemId)
{
    return container_.del(itemId);
}

bool ItemManager::getItem(itemid_t itemId, Item& item)
{
    return container_.getValue(itemId, item);
}

unsigned int ItemManager::itemNum()
{
    return container_.numItems();
}

ItemManager::SDBIterator ItemManager::begin()
{
    return SDBIterator(container_);
}

ItemManager::SDBIterator ItemManager::end()
{
    return SDBIterator();
}

} // namespace sf1r
