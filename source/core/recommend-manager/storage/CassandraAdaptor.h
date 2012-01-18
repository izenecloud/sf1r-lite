/**
 * @file CassandraAdaptor.h
 * @author Jun Jiang
 * @date 2012-01-16
 */

#ifndef CASSANDRA_ADAPTOR_H
#define CASSANDRA_ADAPTOR_H

#include <3rdparty/libcassandra/cassandra.h>

#include <string>
#include <vector>

namespace sf1r
{

class CassandraAdaptor
{
public:
    CassandraAdaptor(const std::string& keyspace, const std::string& columnFamily);

    bool createColumnFamily(const org::apache::cassandra::CfDef& cfDef);
    bool dropColumnFamily(const std::string& columnFamily);

    bool insertColumn(const std::string& key, const std::string& name, const std::string& value);
    bool remove(const std::string& key);
    bool getColumns(const std::string& key, std::vector<org::apache::cassandra::Column>& columns);

private:
    const std::string keyspace_;
    const std::string columnFamily_;

    libcassandra::Cassandra* client_;
};

} // namespace sf1r

#endif // CASSANDRA_ADAPTOR_H
