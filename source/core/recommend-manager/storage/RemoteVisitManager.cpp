#include "RemoteVisitManager.h"

using namespace org::apache::cassandra;

namespace
{
const char* SUPER_COLUMN_NAME_SESSION = "Session";
const char* SUPER_COLUMN_NAME_ITEMS = "Items";
const char* SUB_COLUMN_NAME_SESSION_ID = "session_id";

CfDef createItemCFDef(
    const std::string& keyspace,
    const std::string& columnFamily
)
{
    CfDef def;

    def.__set_keyspace(keyspace);
    def.__set_name(columnFamily);

    def.__set_key_validation_class("UTF8Type");
    def.__set_comparator_type("IntegerType");

    return def;
}

CfDef createSessionCFDef(
    const std::string& keyspace,
    const std::string& columnFamily
)
{
    CfDef def;

    def.__set_keyspace(keyspace);
    def.__set_name(columnFamily);

    def.__set_key_validation_class("UTF8Type");
    def.__set_column_type("Super");
    def.__set_comparator_type("UTF8Type");
    def.__set_subcomparator_type("IntegerType");

    return def;
}

bool convertSessionId(
    const std::vector<Column> subColumns,
    std::string& sessionId
)
{
    const std::string subColumnName = SUB_COLUMN_NAME_SESSION_ID;

    for (std::vector<Column>::const_iterator it = subColumns.begin();
        it != subColumns.end(); ++it)
    {
        const std::string& name = it->name;
        const std::string& value = it->value;

        if (name == subColumnName)
        {
            sessionId = value;
            return true;
        }
    }

    LOG(ERROR) << "not found sub column name " << subColumnName;
    return false;
}

bool convertSuperColumn(
    const SuperColumn& superColumn,
    sf1r::VisitSession& visitSession
)
{
    const std::string& superName = superColumn.name;
    const std::vector<Column> subColumns = superColumn.columns;

    if (superName == SUPER_COLUMN_NAME_SESSION)
    {
        return convertSessionId(subColumns, visitSession.sessionId_);
    }
    else if (superName == SUPER_COLUMN_NAME_ITEMS)
    {
        return sf1r::CassandraAdaptor::convertColumnNames(subColumns, visitSession.itemSet_);
    }

    LOG(ERROR) << "unknown super column name: " << superName;
    return false;
}

bool convertSuperColumns(
    const std::vector<SuperColumn>& superColumns,
    sf1r::VisitSession& visitSession
)
{
    for (std::vector<SuperColumn>::const_iterator it = superColumns.begin();
        it != superColumns.end(); ++it)
    {
        if (! convertSuperColumn(*it, visitSession))
            return false;
    }

    return true;
}

}

namespace sf1r
{

RemoteVisitManager::RemoteVisitManager(
    const std::string& keyspace,
    const std::string& visitColumnFamily,
    const std::string& recommendColumnFamily,
    const std::string& sessionColumnFamily,
    libcassandra::Cassandra* client
)
    : visitCassandra_(createItemCFDef(keyspace, visitColumnFamily), client)
    , recommendCassandra_(createItemCFDef(keyspace, recommendColumnFamily), client)
    , sessionCassandra_(createSessionCFDef(keyspace, sessionColumnFamily), client)
{
}

bool RemoteVisitManager::getVisitItemSet(const std::string& userId, ItemIdSet& itemIdSet)
{
    return visitCassandra_.getColumnNames(userId, itemIdSet);
}

bool RemoteVisitManager::getRecommendItemSet(const std::string& userId, ItemIdSet& itemIdSet)
{
    return recommendCassandra_.getColumnNames(userId, itemIdSet);
}

bool RemoteVisitManager::getVisitSession(const std::string& userId, VisitSession& visitSession)
{
    std::vector<SuperColumn> superColumns;

    return sessionCassandra_.getSuperColumns(userId, superColumns) &&
           convertSuperColumns(superColumns, visitSession);
}

bool RemoteVisitManager::saveVisitItem_(
    const std::string& userId,
    const ItemIdSet& totalItems,
    itemid_t newItem
)
{
    return visitCassandra_.insertColumn(userId, newItem);
}

bool RemoteVisitManager::saveRecommendItem_(
    const std::string& userId,
    const ItemIdSet& totalItems,
    itemid_t newItem
)
{
    return recommendCassandra_.insertColumn(userId, newItem);
}

bool RemoteVisitManager::saveVisitSession_(
    const std::string& userId,
    const VisitSession& visitSession,
    bool isNewSession,
    itemid_t newItem
)
{
    const std::string& key = userId;

    if (isNewSession)
    {
        return sessionCassandra_.remove(key) &&
               sessionCassandra_.insertSuperColumn(key, SUPER_COLUMN_NAME_SESSION, SUB_COLUMN_NAME_SESSION_ID, visitSession.sessionId_) &&
               sessionCassandra_.insertSuperColumn(key, SUPER_COLUMN_NAME_ITEMS, newItem);
    }

    return sessionCassandra_.insertSuperColumn(key, SUPER_COLUMN_NAME_ITEMS, newItem);
}

} // namespace sf1r
