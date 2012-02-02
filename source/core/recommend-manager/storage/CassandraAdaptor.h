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
#include <set>
#include <boost/lexical_cast.hpp>

#include <glog/logging.h>

namespace sf1r
{

class CassandraAdaptor
{
public:
    /**
     * @param columnFamily column family name
     * @param client cassandra client instance
     * @attention if @p client is NULL, meaning the connection with cassandra server failed,
     *            then all member functions in @c CassandraAdaptor would return @c false.
     */
    CassandraAdaptor(const std::string& columnFamily, libcassandra::Cassandra* client);

    /**
     * Create column family from @p cfDef.
     */
    CassandraAdaptor(const org::apache::cassandra::CfDef& cfDef, libcassandra::Cassandra* client);

    bool dropColumnFamily();

    bool remove(const std::string& key);

    bool getColumns(const std::string& key, std::vector<org::apache::cassandra::Column>& columns);
    bool getAllColumns(const std::string& key, std::vector<org::apache::cassandra::Column>& columns);
    bool getSuperColumns(const std::string& key, std::vector<org::apache::cassandra::SuperColumn>& superColumns);

    template <typename ColumnNameType>
    bool getColumnNames(const std::string& key, std::set<ColumnNameType>& nameSet);

    bool insertColumn(const std::string& key, const std::string& name, const std::string& value);

    template <typename ColumnNameType>
    bool insertColumn(const std::string& key, const ColumnNameType& name, const std::string& value = "");

    bool insertSuperColumn(
        const std::string& key,
        const std::string& superColumnName,
        const std::string& subColumnName,
        const std::string& value
    );

    template <typename SubColumnNameType>
    bool insertSuperColumn(
        const std::string& key,
        const std::string& superColumnName,
        const SubColumnNameType& subColumnName,
        const std::string& value = ""
    );

    template <typename ColumnNameType>
    static bool convertColumnNames(const std::vector<org::apache::cassandra::Column>& columns, std::set<ColumnNameType>& nameSet);

private:
    bool createColumnFamily_(const org::apache::cassandra::CfDef& cfDef);

private:
    const std::string columnFamily_;
    libcassandra::Cassandra* client_;
};

template <typename ColumnNameType>
bool CassandraAdaptor::getColumnNames(const std::string& key, std::set<ColumnNameType>& nameSet)
{
    std::vector<org::apache::cassandra::Column> columns;
    if (! getAllColumns(key, columns))
        return false;

    return convertColumnNames(columns, nameSet);
}

template <typename ColumnNameType>
bool CassandraAdaptor::convertColumnNames(const std::vector<org::apache::cassandra::Column>& columns, std::set<ColumnNameType>& nameSet)
{
    try
    {
        for (std::vector<org::apache::cassandra::Column>::const_iterator it = columns.begin();
            it != columns.end(); ++it)
        {
            ColumnNameType name = boost::lexical_cast<ColumnNameType>(it->name);
            nameSet.insert(name);
        }
    }
    catch(const boost::bad_lexical_cast& e)
    {
        LOG(ERROR) << "failed in casting column name, exception: " << e.what();
        return false;
    }

    return true;
}

template <typename ColumnNameType>
bool CassandraAdaptor::insertColumn(const std::string& key, const ColumnNameType& name, const std::string& value)
{
    std::string nameStr = boost::lexical_cast<std::string>(name);
    return insertColumn(key, nameStr, value);
}

template <typename SubColumnNameType>
bool CassandraAdaptor::insertSuperColumn(
    const std::string& key,
    const std::string& superColumnName,
    const SubColumnNameType& subColumnName,
    const std::string& value
)
{
    std::string subNameStr = boost::lexical_cast<std::string>(subColumnName);
    return insertSuperColumn(key, superColumnName, subNameStr, value);
}

} // namespace sf1r

#endif // CASSANDRA_ADAPTOR_H
