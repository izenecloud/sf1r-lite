#include "RemotePurchaseManager.h"

namespace
{
const char* COLUMN_FAMILY_NAME_SUFFIX = "purchase";

org::apache::cassandra::CfDef createCFDef()
{
    org::apache::cassandra::CfDef def;

    def.__set_key_validation_class("UTF8Type");
    def.__set_comparator_type("IntegerType");

    return def;
}

}

namespace sf1r
{

RemotePurchaseManager::RemotePurchaseManager(const std::string& keyspace, const std::string& collection)
    : RemoteStorage(keyspace, collection, COLUMN_FAMILY_NAME_SUFFIX, createCFDef())
{
}

bool RemotePurchaseManager::getPurchaseItemSet(
    const std::string& userId,
    ItemIdSet& itemIdSet
)
{
    return cassandra_.getIntegerColumnNames(userId, itemIdSet);
}

bool RemotePurchaseManager::savePurchaseItem_(
    const std::string& userId,
    const ItemIdSet& totalItems,
    const std::list<itemid_t>& newItems
)
{
    return cassandra_.insertIntegerColumnNames(userId, newItems.begin(), newItems.end());
}

} // namespace sf1r
