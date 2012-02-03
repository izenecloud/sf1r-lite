#include "RemoteRateManager.h"

using namespace org::apache::cassandra;

namespace
{

using namespace sf1r;

void convertItemRateMap(
    const std::vector<Column> columns,
    ItemRateMap& itemRateMap
)
{
    itemid_t itemId = 0;
    rate_t rate = 0;

    for (std::vector<Column>::const_iterator it = columns.begin();
        it != columns.end(); ++it)
    {
        itemId = boost::lexical_cast<itemid_t>(it->name);
        rate = boost::lexical_cast<rate_t>(it->value);

        itemRateMap[itemId] = rate;
    }
}

}

namespace sf1r
{

RemoteRateManager::RemoteRateManager(
    const std::string& keyspace,
    const std::string& columnFamily,
    libcassandra::Cassandra* client
)
    : cassandra_(CassandraAdaptor::createColumnFamilyDef(keyspace, columnFamily,
                 "UTF8Type", "IntegerType"), client)
{
}

bool RemoteRateManager::addRate(
    const std::string& userId,
    itemid_t itemId,
    rate_t rate
)
{
    return cassandra_.insertColumn(userId, itemId, rate);
}

bool RemoteRateManager::removeRate(
    const std::string& userId,
    itemid_t itemId
)
{
    rate_t rate = 0;
    if (! cassandra_.getColumnValue(userId, itemId, rate))
        return false;

    return cassandra_.removeColumn(userId, itemId);
}

bool RemoteRateManager::getItemRateMap(
    const std::string& userId,
    ItemRateMap& itemRateMap
)
{
    std::vector<Column> columns;
    if (! cassandra_.getAllColumns(userId, columns))
        return false;

    try
    {
        convertItemRateMap(columns, itemRateMap);
    }
    catch(const boost::bad_lexical_cast& e)
    {
        LOG(ERROR) << "failed in casting column name or value, exception: " << e.what();
        return false;
    }

    return true;
}

} // namespace sf1r
