/**
 * @file RemoteVisitManager.h
 * @author Jun Jiang
 * @date 2012-01-30
 */

#ifndef REMOTE_VISIT_MANAGER_H
#define REMOTE_VISIT_MANAGER_H

#include "VisitManager.h"
#include "CassandraAdaptor.h"

#include <string>

namespace sf1r
{

class RemoteVisitManager : public VisitManager
{
public:
    RemoteVisitManager(
        const std::string& keyspace,
        const std::string& visitColumnFamily,
        const std::string& recommendColumnFamily,
        const std::string& sessionColumnFamily,
        libcassandra::Cassandra* client
    );

    virtual bool visitRecommendItem(const std::string& userId, itemid_t itemId);

    virtual bool getVisitItemSet(const std::string& userId, ItemIdSet& itemIdSet);

    virtual bool getRecommendItemSet(const std::string& userId, ItemIdSet& itemIdSet);

    virtual bool getVisitSession(const std::string& userId, VisitSession& visitSession);

protected:
    virtual bool saveVisitItem_(const std::string& userId, itemid_t itemId);

    virtual bool saveVisitSession_(
        const std::string& userId,
        const VisitSession& visitSession,
        bool isNewSession,
        itemid_t newItem
    );

private:
    CassandraAdaptor visitCassandra_; // the items visited
    CassandraAdaptor recommendCassandra_; // the items recommended
    CassandraAdaptor sessionCassandra_; // the items in current session
};

} // namespace sf1r

#endif // REMOTE_VISIT_MANAGER_H
