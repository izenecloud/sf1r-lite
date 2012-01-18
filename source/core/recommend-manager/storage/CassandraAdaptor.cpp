#include "CassandraAdaptor.h"
#include <log-manager/CassandraConnection.h>

#include <glog/logging.h>

namespace sf1r
{

CassandraAdaptor::CassandraAdaptor(const std::string& keyspace, const std::string& columnFamily)
    : keyspace_(keyspace)
    , columnFamily_(columnFamily)
    , client_(CassandraConnection::instance().getCassandraClient(keyspace_))
{
    if (! client_)
    {
        LOG(ERROR) << "failed to connect cassandra server, keyspace: " << keyspace_
                   << ", column family: " << columnFamily_;
    }
}

bool CassandraAdaptor::createColumnFamily(const org::apache::cassandra::CfDef& cfDef)
{
    if (! client_)
        return false;

    try
    {
        client_->createColumnFamily(cfDef);
    }
    CATCH_CASSANDRA_EXCEPTION("[Cassandra::createColumnFamily] error: ")

    return true;
}

bool CassandraAdaptor::dropColumnFamily(const std::string& columnFamily)
{
    if (! client_)
        return false;

    try
    {
        client_->dropColumnFamily(columnFamily);
    }
    CATCH_CASSANDRA_EXCEPTION("[Cassandra::dropColumnFamily] error: ")

    return true;
}

bool CassandraAdaptor::insertColumn(const std::string& key, const std::string& name, const std::string& value)
{
    if (! client_)
        return false;

    try
    {
        client_->insertColumn(value, key, columnFamily_, name);
    }
    CATCH_CASSANDRA_EXCEPTION("[Cassandra::insertColumn] error: ")

    return true;
}

bool CassandraAdaptor::remove(const std::string& key)
{
    if (! client_)
        return false;

    org::apache::cassandra::ColumnPath colPath;
    colPath.__set_column_family(columnFamily_);

    try
    {
        client_->remove(key, colPath);
    }
    CATCH_CASSANDRA_EXCEPTION("[Cassandra::remove] error: ")

    return true;
}

bool CassandraAdaptor::getColumns(const std::string& key, std::vector<org::apache::cassandra::Column>& columns)
{
    if (! client_)
        return false;

    org::apache::cassandra::ColumnParent colParent;
    colParent.__set_column_family(columnFamily_);

    org::apache::cassandra::SlicePredicate pred;
    org::apache::cassandra::SliceRange sliceRange;
    pred.__set_slice_range(sliceRange);

    try
    {
        client_->getSlice(columns, key, colParent, pred);
    }
    CATCH_CASSANDRA_EXCEPTION("[Cassandra::getSlice] error: ")

    return true;
}

} // namespace sf1r
