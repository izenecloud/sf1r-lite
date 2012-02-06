#include "LocalEventManager.h"

#include <glog/logging.h>

namespace sf1r
{

LocalEventManager::LocalEventManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

void LocalEventManager::flush()
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

bool LocalEventManager::addEvent(
    const std::string& eventStr,
    const std::string& userId,
    itemid_t itemId
)
{
    EventItemMap eventItemMap;
    if (! getEvent(userId, eventItemMap))
        return false;

    if (! eventItemMap[eventStr].insert(itemId).second)
    {
        LOG(WARNING) << "event already exists, eventStr: " << eventStr
                     << ", userId: " << userId
                     << ", itemId: " << itemId;
        return true;
    }

    return saveEventItemMap_(userId, eventItemMap);
}

bool LocalEventManager::removeEvent(
    const std::string& eventStr,
    const std::string& userId,
    itemid_t itemId
)
{
    EventItemMap eventItemMap;
    if (! getEvent(userId, eventItemMap))
        return false;

    if (eventItemMap[eventStr].erase(itemId) == 0)
    {
        LOG(ERROR) << "try to remove non-existing event, eventStr: " << eventStr
                     << ", userId: " << userId
                     << ", itemId: " << itemId;
        return false;
    }

    return saveEventItemMap_(userId, eventItemMap);
}

bool LocalEventManager::getEvent(
    const std::string& userId,
    EventItemMap& eventItemMap
)
{
    bool result = false;

    try
    {
        eventItemMap.clear();
        container_.getValue(userId, eventItemMap);
        result = true;
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
}

bool LocalEventManager::saveEventItemMap_(
    const std::string& userId,
    const EventItemMap& eventItemMap
)
{
    bool result = false;

    try
    {
        result = container_.update(userId, eventItemMap);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return result;
}

} // namespace sf1r
