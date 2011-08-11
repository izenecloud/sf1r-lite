#include "EventManager.h"

#include <glog/logging.h>

namespace sf1r
{

EventManager::EventManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

EventManager::~EventManager()
{
    flush();
}

void EventManager::flush()
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

bool EventManager::addEvent(
    const std::string& eventStr,
    userid_t userId,
    itemid_t itemId
)
{
    EventItemMap eventItemMap;
    container_.getValue(userId, eventItemMap);

    if (eventItemMap[eventStr].insert(itemId).second == false)
    {
        LOG(WARNING) << "event already exists, eventStr: " << eventStr
                     << ", userId: " << userId
                     << ", itemId: " << itemId;
        return true;
    }

    try
    {
        if (container_.update(userId, eventItemMap) == false)
        {
            return false;
        }
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return true;
}

bool EventManager::removeEvent(
    const std::string& eventStr,
    userid_t userId,
    itemid_t itemId
)
{
    EventItemMap eventItemMap;
    container_.getValue(userId, eventItemMap);

    if (eventItemMap[eventStr].erase(itemId) == 0)
    {
        LOG(ERROR) << "try to remove non-existing event, eventStr: " << eventStr
                     << ", userId: " << userId
                     << ", itemId: " << itemId;
        return false;
    }

    try
    {
        if (container_.update(userId, eventItemMap) == false)
        {
            return false;
        }
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return true;
}

bool EventManager::getEvent(
    userid_t userId,
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

} // namespace sf1r
