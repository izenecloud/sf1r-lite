/**
 * @file RemoteEventManager.h
 * @author Jun Jiang
 * @date 2012-02-03
 */

#ifndef REMOTE_EVENT_MANAGER_H
#define REMOTE_EVENT_MANAGER_H

#include "EventManager.h"
#include "CassandraAdaptor.h"

#include <string>

namespace sf1r
{

class RemoteEventManager : public EventManager
{
public:
    RemoteEventManager(
        const std::string& keyspace,
        const std::string& columnFamily,
        libcassandra::Cassandra* client
    );

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
    CassandraAdaptor cassandra_;
};

} // namespace sf1r

#endif // REMOTE_EVENT_MANAGER_H
