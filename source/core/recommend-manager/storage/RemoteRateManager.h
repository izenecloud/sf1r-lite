/**
 * @file RemoteRateManager.h
 * @author Jun Jiang
 * @date 2012-02-03
 */

#ifndef REMOTE_RATE_MANAGER_H
#define REMOTE_RATE_MANAGER_H

#include "RateManager.h"
#include "CassandraAdaptor.h"

#include <string>

namespace sf1r
{

class RemoteRateManager : public RateManager
{
public:
    RemoteRateManager(
        const std::string& keyspace,
        const std::string& columnFamily,
        libcassandra::Cassandra* client
    );

    virtual bool addRate(
        const std::string& userId,
        itemid_t itemId,
        rate_t rate
    );

    virtual bool removeRate(
        const std::string& userId,
        itemid_t itemId
    );

    virtual bool getItemRateMap(
        const std::string& userId,
        ItemRateMap& itemRateMap
    );

private:
    CassandraAdaptor cassandra_;
};

} // namespace sf1r

#endif // REMOTE_RATE_MANAGER_H
