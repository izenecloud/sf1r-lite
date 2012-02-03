#include "RemoteEventManager.h"

using namespace org::apache::cassandra;

namespace
{
using namespace sf1r;

bool convertEventItemMap(
    const std::vector<SuperColumn>& superColumns,
    EventManager::EventItemMap& eventItemMap
)
{
    for (std::vector<SuperColumn>::const_iterator it = superColumns.begin();
        it != superColumns.end(); ++it)
    {
        const std::string& superColumnName = it->name;
        const std::vector<Column>& subColumns = it->columns;

        if (! sf1r::CassandraAdaptor::convertColumnNames(subColumns,
                                                         eventItemMap[superColumnName]))
        {
            return false;
        }
    }

    return true;
}

}

namespace sf1r
{

RemoteEventManager::RemoteEventManager(
    const std::string& keyspace,
    const std::string& columnFamily,
    libcassandra::Cassandra* client
)
    : cassandra_(CassandraAdaptor::createColumnFamilyDef(keyspace, columnFamily,
                 "UTF8Type", "UTF8Type", "IntegerType"), client)
{
}

bool RemoteEventManager::addEvent(
    const std::string& eventStr,
    const std::string& userId,
    itemid_t itemId
)
{
    return cassandra_.insertSuperColumn(userId, eventStr, itemId);
}

bool RemoteEventManager::removeEvent(
    const std::string& eventStr,
    const std::string& userId,
    itemid_t itemId
)
{
    std::string value;
    if (! cassandra_.getColumnValue(userId, itemId, value, eventStr))
        return false;

    return cassandra_.removeColumn(userId, itemId, eventStr);
}

bool RemoteEventManager::getEvent(
    const std::string& userId,
    EventItemMap& eventItemMap
)
{
    std::vector<SuperColumn> superColumns;

    return cassandra_.getSuperColumns(userId, superColumns) &&
           convertEventItemMap(superColumns, eventItemMap);
}

} // namespace sf1r
