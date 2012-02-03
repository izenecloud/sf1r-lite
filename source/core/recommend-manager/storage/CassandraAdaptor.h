/**
 * @file CassandraAdaptor.h
 * @author Jun Jiang
 * @date 2012-01-16
 */

#ifndef CASSANDRA_ADAPTOR_H
#define CASSANDRA_ADAPTOR_H

#include "CassandraAdaptorTraits.h"
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

    /**
     * Insert a column.
     * @param key the row key
     * @param name the column name
     * @return true for success, false for failure
     * @note the column value would be empty string
     */
    template <typename ColumnNameType>
    bool insertColumn(
        const std::string& key,
        const ColumnNameType& name
    );

    /**
     * Insert a column.
     * @param key the row key
     * @param name the column name
     * @param value the column value
     * @return true for success, false for failure
     */
    template <typename ColumnNameType, typename ColumnValueType>
    bool insertColumn(
        const std::string& key,
        const ColumnNameType& name,
        const ColumnValueType& value
    );

    template <typename SubColumnNameType>
    bool insertSuperColumn(
        const std::string& key,
        const std::string& superColumnName,
        const SubColumnNameType& subColumnName,
        const std::string& value = ""
    );

    template <typename ColumnNameType>
    bool removeColumn(
        const std::string& key,
        const ColumnNameType& columnName,
        const std::string& superColumnName = ""
    );

    template <typename ColumnNameType, typename ColumnValueType>
    bool getColumnValue(
        const std::string& key,
        const ColumnNameType& columnName,
        ColumnValueType& value,
        const std::string& superColumnName = ""
    );

    template <typename ColumnNameType>
    bool getColumnValue(
        const std::string& key,
        const ColumnNameType& columnName,
        std::string& value,
        const std::string& superColumnName = ""
    );

    template <typename ColumnNameContainer>
    bool getColumnNames(
        const std::string& key,
        ColumnNameContainer& nameContainer
    );

    template <typename ColumnNameContainer>
    static bool convertColumnNames(
        const std::vector<org::apache::cassandra::Column>& columns,
        ColumnNameContainer& nameContainer
    );

    /**
     * Create column family definition.
     * @param subcomparator_type sub comparator type, its default value is empty.
     * if empty, it would create a standard column family definition,
     * otherwise, it would create a super column family definition with the sub comparator type.
     * @return column family definition
     */
    static org::apache::cassandra::CfDef createColumnFamilyDef(
        const std::string& keyspace,
        const std::string& column_family_name,
        const std::string& key_validation_class,
        const std::string& comparator_type,
        const std::string& subcomparator_type = ""
    );

private:
    bool createColumnFamily_(const org::apache::cassandra::CfDef& cfDef);

    bool insertColumnImpl_(
        const std::string& key,
        const std::string& name,
        const std::string& value
    );

    bool insertSuperColumnImpl_(
        const std::string& key,
        const std::string& superColumnName,
        const std::string& subColumnName,
        const std::string& value
    );

    bool removeColumnImpl_(
        const std::string& key,
        const std::string& superColumnName,
        const std::string& subColumnName
    );

    bool getColumnValueImpl_(
        const std::string& key,
        const std::string& superColumnName,
        const std::string& subColumnName,
        std::string& value
    );

private:
    const std::string columnFamily_;
    libcassandra::Cassandra* client_;
};

template <typename ColumnNameType>
inline bool CassandraAdaptor::insertColumn(
    const std::string& key,
    const ColumnNameType& name
)
{
    const std::string& nameStr = CassandraAdaptorTraits::convertToString(name);

    return insertColumnImpl_(key, nameStr, "");
}

template <typename ColumnNameType, typename ColumnValueType>
inline bool CassandraAdaptor::insertColumn(
    const std::string& key,
    const ColumnNameType& name,
    const ColumnValueType& value
)
{
    const std::string& nameStr = CassandraAdaptorTraits::convertToString(name);
    const std::string& valueStr = CassandraAdaptorTraits::convertToString(value);

    return insertColumnImpl_(key, nameStr, valueStr);
}

template <typename SubColumnNameType>
inline bool CassandraAdaptor::insertSuperColumn(
    const std::string& key,
    const std::string& superColumnName,
    const SubColumnNameType& subColumnName,
    const std::string& value
)
{
    const std::string& subNameStr = CassandraAdaptorTraits::convertToString(subColumnName);

    return insertSuperColumnImpl_(key, superColumnName, subNameStr, value);
}

template <typename ColumnNameType>
inline bool CassandraAdaptor::removeColumn(
    const std::string& key,
    const ColumnNameType& columnName,
    const std::string& superColumnName
)
{
    const std::string& nameStr = CassandraAdaptorTraits::convertToString(columnName);

    return removeColumnImpl_(key, superColumnName, nameStr);
}

template <typename ColumnNameType, typename ColumnValueType>
inline bool CassandraAdaptor::getColumnValue(
    const std::string& key,
    const ColumnNameType& columnName,
    ColumnValueType& value,
    const std::string& superColumnName
)
{
    const std::string& nameStr = CassandraAdaptorTraits::convertToString(columnName);
    std::string valueStr;

    if (! getColumnValueImpl_(key, superColumnName, nameStr, valueStr))
        return false;

    try
    {
        value = boost::lexical_cast<ColumnValueType>(valueStr);
    }
    catch(const boost::bad_lexical_cast& e)
    {
        LOG(ERROR) << "failed in casting column value, exception: " << e.what();
        return false;
    }

    return true;
}

template <typename ColumnNameType>
inline bool CassandraAdaptor::getColumnValue(
    const std::string& key,
    const ColumnNameType& columnName,
    std::string& value,
    const std::string& superColumnName
)
{
    const std::string& nameStr = CassandraAdaptorTraits::convertToString(columnName);

    return getColumnValueImpl_(key, superColumnName, nameStr, value);
}

template <typename ColumnNameContainer>
inline bool CassandraAdaptor::getColumnNames(
    const std::string& key,
    ColumnNameContainer& nameContainer
)
{
    std::vector<org::apache::cassandra::Column> columns;

    if (! getAllColumns(key, columns))
        return false;

    return convertColumnNames(columns, nameContainer);
}

template <typename ColumnNameContainer>
inline bool CassandraAdaptor::convertColumnNames(
    const std::vector<org::apache::cassandra::Column>& columns,
    ColumnNameContainer& nameContainer
)
{
    typedef typename ColumnNameContainer::value_type ColumnNameType;

    try
    {
        for (std::vector<org::apache::cassandra::Column>::const_iterator it = columns.begin();
            it != columns.end(); ++it)
        {
            ColumnNameType name = boost::lexical_cast<ColumnNameType>(it->name);
            CassandraAdaptorTraits::insertContainer(nameContainer, name);
        }
    }
    catch(const boost::bad_lexical_cast& e)
    {
        LOG(ERROR) << "failed in casting column name, exception: " << e.what();
        return false;
    }

    return true;
}

} // namespace sf1r

#endif // CASSANDRA_ADAPTOR_H
