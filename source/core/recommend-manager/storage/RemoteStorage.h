/**
 * @file RemoteStorage.h
 * @author Jun Jiang
 * @date 2012-01-10
 */

#ifndef REMOTE_STORAGE_H
#define REMOTE_STORAGE_H

#include "CassandraAdaptor.h"

#include <3rdparty/libcassandra/cassandra.h>

#include <string>

namespace sf1r
{

class RemoteStorage
{
public:
    RemoteStorage(
        const std::string& keyspace,
        const std::string& collection,
        const std::string& columnFamilySuffix,
        const org::apache::cassandra::CfDef& cfDef
    );

    virtual ~RemoteStorage() {}

protected:
    const std::string keyspace_;
    const std::string collection_;
    const std::string columnFamily_;

    CassandraAdaptor cassandra_;
};

} // namespace sf1r

#endif // REMOTE_STORAGE_H
