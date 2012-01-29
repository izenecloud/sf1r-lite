/**
 * @file EventManager.h
 * @author Jun Jiang
 * @date 2011-08-09
 */

#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include "RecTypes.h"
#include <sdb/SequentialDB.h>

#include <string>
#include <map>

#include <boost/serialization/map.hpp> // serialize std::map
#include <boost/serialization/set.hpp> // serialize ItemIdSet

namespace sf1r
{

class EventManager
{
public:
    /** event type => item id set */
    typedef std::map<std::string, ItemIdSet> EventItemMap;

    EventManager(const std::string& path);

    ~EventManager();

    void flush();

    /**
     * Add an event.
     * @param eventStr the event type
     * @param userId the user id
     * @param itemId the item id
     * @return true for succcess, false for failure
     */
    bool addEvent(
        const std::string& eventStr,
        const std::string& userId,
        itemid_t itemId
    );

    /**
     * Remove an event.
     * @param eventStr the event type
     * @param userId the user id
     * @param itemId the item id
     * @return true for succcess, false for failure
     */
    bool removeEvent(
        const std::string& eventStr,
        const std::string& userId,
        itemid_t itemId
    );

    /**
     * Get all events by @p userId.
     * @param userId user id
     * @param eventItemMap store all events by @p userId
     * @return true for success, false for error happened.
     */
    bool getEvent(
        const std::string& userId,
        EventItemMap& eventItemMap
    );

private:
    typedef izenelib::sdb::unordered_sdb_tc<std::string, EventItemMap, ReadWriteLock> SDBType;
    SDBType container_;
};

} // namespace sf1r

#endif // EVENT_MANAGER_H
