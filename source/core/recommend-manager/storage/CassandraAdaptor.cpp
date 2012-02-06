#include "CassandraAdaptor.h"
#include <log-manager/CassandraConnection.h>

#include <cassert>

using namespace org::apache::cassandra;

namespace sf1r
{

CassandraAdaptor::CassandraAdaptor(const std::string& columnFamily, libcassandra::Cassandra* client)
    : columnFamily_(columnFamily)
    , client_(client)
{
}

CassandraAdaptor::CassandraAdaptor(const org::apache::cassandra::CfDef& cfDef, libcassandra::Cassandra* client)
    : columnFamily_(cfDef.name)
    , client_(client)
{
    createColumnFamily_(cfDef);
}

bool CassandraAdaptor::createColumnFamily_(const org::apache::cassandra::CfDef& cfDef)
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

bool CassandraAdaptor::dropColumnFamily()
{
    if (! client_)
        return false;

    try
    {
        client_->dropColumnFamily(columnFamily_);
    }
    CATCH_CASSANDRA_EXCEPTION("[Cassandra::dropColumnFamily] error: ")

    return true;
}

bool CassandraAdaptor::remove(const std::string& key)
{
    if (! client_)
        return false;

    ColumnPath colPath;
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

    ColumnParent colParent;
    colParent.__set_column_family(columnFamily_);

    SlicePredicate pred;
    SliceRange sliceRange;
    pred.__set_slice_range(sliceRange);

    try
    {
        client_->getSlice(columns, key, colParent, pred);
    }
    CATCH_CASSANDRA_EXCEPTION("[Cassandra::getSlice] error: ")

    return true;
}

bool CassandraAdaptor::getAllColumns(const std::string& key, std::vector<org::apache::cassandra::Column>& columns)
{
    if (! client_)
        return false;

    ColumnParent colParent;
    colParent.__set_column_family(columnFamily_);

    SlicePredicate pred;
    SliceRange sliceRange;
    pred.__set_slice_range(sliceRange);

    std::string lastColumn;
    std::vector<Column> slice;
    while (true)
    {
        pred.slice_range.__set_start(lastColumn);
        slice.clear();

        try
        {
            client_->getSlice(slice, key, colParent, pred);
        }
        CATCH_CASSANDRA_EXCEPTION("[Cassandra::getSlice] error: ")

        std::vector<Column>::const_iterator beginIt = slice.begin();
        std::vector<Column>::const_iterator endIt = slice.end();

        if (! lastColumn.empty())
        {
            assert(beginIt != endIt);
            ++beginIt;
        }

        if (beginIt == endIt)
            break;

        columns.insert(columns.end(), beginIt, endIt);
        lastColumn = slice.back().name;
    }

    return true;
}

bool CassandraAdaptor::getSuperColumns(const std::string& key, std::vector<org::apache::cassandra::SuperColumn>& superColumns)
{
    if (! client_)
        return false;

    ColumnParent colParent;
    colParent.__set_column_family(columnFamily_);

    SlicePredicate pred;
    SliceRange sliceRange;
    pred.__set_slice_range(sliceRange);

    try
    {
        client_->getSuperSlice(superColumns, key, colParent, pred);
    }
    CATCH_CASSANDRA_EXCEPTION("[Cassandra::getSuperSlice] error: ")

    return true;
}

bool CassandraAdaptor::insertColumnImpl_(
    const std::string& key,
    const std::string& name,
    const std::string& value
)
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

bool CassandraAdaptor::insertSuperColumnImpl_(
    const std::string& key,
    const std::string& superColumnName,
    const std::string& subColumnName,
    const std::string& value
)
{
    if (! client_)
        return false;

    try
    {
        client_->insertColumn(value, key, columnFamily_, superColumnName, subColumnName);
    }
    CATCH_CASSANDRA_EXCEPTION("[Cassandra::insertColumn] (super column) error: ")

    return true;
}

bool CassandraAdaptor::removeColumnImpl_(
    const std::string& key,
    const std::string& superColumnName,
    const std::string& subColumnName
)
{
    if (! client_)
        return false;

    try
    {
        client_->removeColumn(key, columnFamily_, superColumnName, subColumnName);
    }
    CATCH_CASSANDRA_EXCEPTION("[Cassandra::removeColumn] error: ")

    return true;
}

bool CassandraAdaptor::getColumnValueImpl_(
    const std::string& key,
    const std::string& superColumnName,
    const std::string& subColumnName,
    std::string& value
)
{
    if (! client_)
        return false;

    try
    {
        client_->getColumnValue(value, key, columnFamily_,
                                superColumnName, subColumnName);
    }
    CATCH_CASSANDRA_EXCEPTION("[Cassandra::getColumnValue] error: ")

    return true;
}

org::apache::cassandra::CfDef CassandraAdaptor::createColumnFamilyDef(
    const std::string& keyspace,
    const std::string& column_family_name,
    const std::string& key_validation_class,
    const std::string& comparator_type,
    const std::string& subcomparator_type
)
{
    CfDef def;

    def.__set_keyspace(keyspace);
    def.__set_name(column_family_name);
    def.__set_key_validation_class(key_validation_class);
    def.__set_comparator_type(comparator_type);

    if (! subcomparator_type.empty())
    {
        def.__set_column_type("Super");
        def.__set_subcomparator_type(subcomparator_type);
    }

    return def;
}

} // namespace sf1r
