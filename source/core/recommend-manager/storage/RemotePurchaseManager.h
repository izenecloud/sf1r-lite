/**
 * @file RemotePurchaseManager.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef REMOTE_PURCHASE_MANAGER_H
#define REMOTE_PURCHASE_MANAGER_H

#include "PurchaseManager.h"
#include "CassandraAdaptor.h"

#include <string>

namespace sf1r
{

class RemotePurchaseManager : public PurchaseManager
{
public:
    RemotePurchaseManager(
        const std::string& keyspace,
        const std::string& columnFamily,
        libcassandra::Cassandra* client
    );

    virtual bool getPurchaseItemSet(
        const std::string& userId,
        ItemIdSet& itemIdSet
    );

protected:
    virtual bool savePurchaseItem_(
        const std::string& userId,
        const ItemIdSet& totalItems,
        const std::list<itemid_t>& newItems
    );

private:
    CassandraAdaptor cassandra_;
};

} // namespace sf1r

#endif // REMOTE_PURCHASE_MANAGER_H
