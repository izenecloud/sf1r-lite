/**
 * @file LocalEventManager.h
 * @author Jun Jiang
 * @date 2012-02-03
 */

#ifndef LOCAL_EVENT_MANAGER_H
#define LOCAL_EVENT_MANAGER_H

#include "EventManager.h"
#include <sdb/SequentialDB.h>

#include <string>
#include <map>

#include <boost/serialization/map.hpp> // serialize std::map
#include <boost/serialization/set.hpp> // serialize ItemIdSet

namespace sf1r
{

class LocalEventManager : public EventManager
{
public:
    LocalEventManager(const std::string& path);

    virtual void flush();

    virtual bool addEvent(
        const std::string& eventStr,
        const std::string& userId,
        itemid_t itemId
    );

    virtual bool removeEvent(
        const std::string& eventStr,
        const std::string& userId,
        itemid_t itemId
    );

    virtual bool getEvent(
        const std::string& userId,
        EventItemMap& eventItemMap
    );

private:
    bool saveEventItemMap_(
        const std::string& userId,
        const EventItemMap& eventItemMap
    );

private:
    typedef izenelib::sdb::unordered_sdb_tc<std::string, EventItemMap,
                                            ReadWriteLock> SDBType;
    SDBType container_;
};

} // namespace sf1r

#endif // LOCAL_EVENT_MANAGER_H
