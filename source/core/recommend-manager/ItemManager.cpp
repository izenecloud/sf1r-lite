#include "ItemManager.h"

#include <glog/logging.h>

#include <fstream>
#include <boost/thread/locks.hpp>

namespace sf1r
{

ItemManager::ItemManager(const std::string& path, const std::string& maxIdPath)
    : container_(path)
    , maxItemId_(0)
    , maxIdPath_(maxIdPath)
{
    container_.open();

    std::ifstream ifs(maxIdPath_.c_str());
    if (ifs)
    {
        ifs >> maxItemId_;
    }
}

ItemManager::~ItemManager()
{
    flush();
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

    std::ofstream ofs(maxIdPath_.c_str());
    if (ofs)
    {
        ofs << maxItemId_;
    }
    else
    {
        LOG(ERROR) << "failed to write file " << maxIdPath_;
    }
}

bool ItemManager::addItem(itemid_t itemId, const Item& item)
{
    bool result = false;
    try
    {
        result = container_.insertValue(itemId, item);

        if (result)
        {
            boost::mutex::scoped_lock lock(maxIdMutex_);
            if (itemId > maxItemId_)
            {
                maxItemId_ = itemId;
            }
        }
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::insertValue(): " << e.what();
    }

    return result;
}

bool ItemManager::updateItem(itemid_t itemId, const Item& item)
{
    bool result = false;
    try
    {
        result = container_.update(itemId, item);
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

bool ItemManager::hasItem(itemid_t itemId)
{
    bool result = false;
    try
    {
        result = container_.hasKey(itemId);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::hasKey(): " << e.what();
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
