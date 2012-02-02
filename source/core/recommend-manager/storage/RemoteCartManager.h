/**
 * @file RemoteCartManager.h
 * @author Jun Jiang
 * @date 2012-02-01
 */

#ifndef REMOTE_CART_MANAGER_H
#define REMOTE_CART_MANAGER_H

#include "CartManager.h"
#include "CassandraAdaptor.h"

#include <string>

namespace sf1r
{

class RemoteCartManager : public CartManager
{
public:
    RemoteCartManager(
        const std::string& keyspace,
        const std::string& columnFamily,
        libcassandra::Cassandra* client
    );

    virtual bool updateCart(
        const std::string& userId,
        const std::vector<itemid_t>& itemVec
    );

    virtual bool getCart(
        const std::string& userId,
        std::vector<itemid_t>& itemVec
    );

private:
    CassandraAdaptor cassandra_;
};

} // namespace sf1r

#endif // REMOTE_CART_MANAGER_H
