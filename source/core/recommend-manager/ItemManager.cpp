#include "ItemManager.h"

#include <glog/logging.h>

namespace sf1r
{

ItemManager::ItemManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

void ItemManager::flush()
{
    try
    {
        container_.flush();
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::flush(): " << e.what();
    }
}

bool ItemManager::addItem(const Item& item)
{
    bool result = false;
    try
    {
        result = container_.insertValue(item.id_, item);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::insertValue(): " << e.what();
    }

    return result;
}

bool ItemManager::updateItem(const Item& item)
{
    bool result = false;
    try
    {
        result = container_.update(item.id_, item);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return result;
}

bool ItemManager::removeItem(itemid_t itemId)
{
    bool result = false;
    try
    {
        result = container_.del(itemId);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::del(): " << e.what();
    }

    return result;
}

bool ItemManager::getItem(itemid_t itemId, Item& item)
{
    bool result = false;
    try
    {
        result = container_.getValue(itemId, item);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
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
