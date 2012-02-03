#include "LocalRateManager.h"

#include <glog/logging.h>

namespace sf1r
{

LocalRateManager::LocalRateManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

void LocalRateManager::flush()
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

bool LocalRateManager::addRate(
    const std::string& userId,
    itemid_t itemId,
    rate_t rate
)
{
    ItemRateMap itemRateMap;
    if (! getItemRateMap(userId, itemRateMap))
        return false;

    itemRateMap[itemId] = rate;

    return saveItemRateMap_(userId, itemRateMap);
}

bool LocalRateManager::removeRate(
    const std::string& userId,
    itemid_t itemId
)
{
    ItemRateMap itemRateMap;
    if (! getItemRateMap(userId, itemRateMap))
        return false;

    if (itemRateMap.erase(itemId) == 0)
    {
        LOG(ERROR) << "try to remove non-existing rate, userId: " << userId
                   << ", itemId: " << itemId;
        return false;
    }

    return saveItemRateMap_(userId, itemRateMap);
}

bool LocalRateManager::getItemRateMap(
    const std::string& userId,
    ItemRateMap& itemRateMap
)
{
    bool result = false;

    try
    {
        itemRateMap.clear();
        container_.getValue(userId, itemRateMap);
        result = true;
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
}

bool LocalRateManager::saveItemRateMap_(
    const std::string& userId,
    const ItemRateMap& itemRateMap
)
{
    bool result = false;

    try
    {
        result = container_.update(userId, itemRateMap);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return result;
}

} // namespace sf1r
