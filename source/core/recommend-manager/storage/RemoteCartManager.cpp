#include "RemoteCartManager.h"

namespace sf1r
{

RemoteCartManager::RemoteCartManager(
    const std::string& keyspace,
    const std::string& columnFamily,
    libcassandra::Cassandra* client
)
    : cassandra_(CassandraAdaptor::createColumnFamilyDef(keyspace, columnFamily,
                 "UTF8Type", "IntegerType"), client)
{
}

bool RemoteCartManager::getCart(
    const std::string& userId,
    std::vector<itemid_t>& itemVec
)
{
    return cassandra_.getColumnNames(userId, itemVec);
}

bool RemoteCartManager::updateCart(
    const std::string& userId,
    const std::vector<itemid_t>& itemVec
)
{
    const std::string& key = userId;

    if (! cassandra_.remove(key))
        return false;

    for (std::vector<itemid_t>::const_iterator it = itemVec.begin();
        it != itemVec.end(); ++it)
    {
        if (! cassandra_.insertColumn(userId, *it))
            return false;
    }

    return true;
}

} // namespace sf1r
