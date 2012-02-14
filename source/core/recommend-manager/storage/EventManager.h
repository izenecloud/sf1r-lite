/**
 * @file EventManager.h
 * @author Jun Jiang
 * @date 2012-02-03
 */

#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include "../common/RecTypes.h"

#include <string>
#include <map>

namespace sf1r
{

class EventManager
{
public:
    /** event type => item id set */
    typedef std::map<std::string, ItemIdSet> EventItemMap;

    virtual ~EventManager() {}

    virtual void flush() {}

    /**
     * Add an event.
     * @param eventStr the event type
     * @param userId the user id
     * @param itemId the item id
     * @return true for succcess, false for failure
     */
    virtual bool addEvent(
        const std::string& eventStr,
        const std::string& userId,
        itemid_t itemId
    ) = 0;

    /**
     * Remove an event.
     * @param eventStr the event type
     * @param userId the user id
     * @param itemId the item id
     * @return true for succcess, false for failure
     */
    virtual bool removeEvent(
        const std::string& eventStr,
        const std::string& userId,
        itemid_t itemId
    ) = 0;

    /**
     * Get all events by @p userId.
     * @param userId user id
     * @param eventItemMap store all events by @p userId
     * @return true for success, false for error happened.
     */
    virtual bool getEvent(
        const std::string& userId,
        EventItemMap& eventItemMap
    ) = 0;
};

} // namespace sf1r

#endif // EVENT_MANAGER_H
