#include "RateManager.h"

#include <glog/logging.h>

namespace sf1r
{

RateManager::RateManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

RateManager::~RateManager()
{
    flush();
}

void RateManager::flush()
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

bool RateManager::addRate(
    const std::string& userId,
    itemid_t itemId,
    rate_t rate
)
{
    ItemRateMap itemRateMap;
    container_.getValue(userId, itemRateMap);

    itemRateMap[itemId] = rate;

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

bool RateManager::removeRate(
    const std::string& userId,
    itemid_t itemId
)
{
    ItemRateMap itemRateMap;
    container_.getValue(userId, itemRateMap);

    if (itemRateMap.erase(itemId) == 0)
    {
        LOG(ERROR) << "try to remove non-existing rate, userId: " << userId
                   << ", itemId: " << itemId;
        return false;
    }

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

bool RateManager::getItemRateMap(
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

} // namespace sf1r
