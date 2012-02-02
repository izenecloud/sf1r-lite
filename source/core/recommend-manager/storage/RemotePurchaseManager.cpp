#include "RemotePurchaseManager.h"

namespace
{

org::apache::cassandra::CfDef createCFDef(
    const std::string& keyspace,
    const std::string& columnFamily
)
{
    org::apache::cassandra::CfDef def;

    def.__set_keyspace(keyspace);
    def.__set_name(columnFamily);

    def.__set_key_validation_class("UTF8Type");
    def.__set_comparator_type("IntegerType");

    return def;
}

}

namespace sf1r
{

RemotePurchaseManager::RemotePurchaseManager(
    const std::string& keyspace,
    const std::string& columnFamily,
    libcassandra::Cassandra* client
)
    : cassandra_(createCFDef(keyspace, columnFamily), client)
{
}

bool RemotePurchaseManager::getPurchaseItemSet(
    const std::string& userId,
    ItemIdSet& itemIdSet
)
{
    return cassandra_.getColumnNames(userId, itemIdSet);
}

bool RemotePurchaseManager::savePurchaseItem_(
    const std::string& userId,
    const ItemIdSet& totalItems,
    const std::list<itemid_t>& newItems
)
{
    for (std::list<itemid_t>::const_iterator it = newItems.begin();
        it != newItems.end(); ++it)
    {
        if (! cassandra_.insertColumn(userId, *it))
            return false;
    }

    return true;
}

} // namespace sf1r
