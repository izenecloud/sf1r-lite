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

    bool insertColumn(const std::string& key, const std::string& name, const std::string& value);
    bool remove(const std::string& key);

    bool getColumns(const std::string& key, std::vector<org::apache::cassandra::Column>& columns);
    bool getAllColumns(const std::string& key, std::vector<org::apache::cassandra::Column>& columns);

    template <typename InputIterator>
    bool insertIntegerColumnNames(const std::string& key, InputIterator first, InputIterator last);

    template <typename IntegerType>
    bool getIntegerColumnNames(const std::string& key, std::set<IntegerType>& nameSet);

private:
    bool createColumnFamily_(const org::apache::cassandra::CfDef& cfDef);

private:
    const std::string columnFamily_;
    libcassandra::Cassandra* client_;
};

template <typename InputIterator>
bool CassandraAdaptor::insertIntegerColumnNames(const std::string& key, InputIterator first, InputIterator last)
{
    std::string name;

    for (InputIterator it = first; it != last; ++it)
    {
        name = boost::lexical_cast<std::string>(*it);
        if (! insertColumn(key, name, ""))
            return false;
    }

    return true;
}

template <typename IntegerType>
bool CassandraAdaptor::getIntegerColumnNames(const std::string& key, std::set<IntegerType>& nameSet)
{
    std::vector<org::apache::cassandra::Column> columns;
    if (! getAllColumns(key, columns))
        return false;

    try
    {
        for (std::vector<org::apache::cassandra::Column>::const_iterator it = columns.begin();
            it != columns.end(); ++it)
        {
            IntegerType integerName = boost::lexical_cast<IntegerType>(it->name);
            nameSet.insert(integerName);
        }
    }
    catch(const boost::bad_lexical_cast& e)
    {
        LOG(ERROR) << "failed in casting column name to numeric value, exception: " << e.what();
        return false;
    }

    return true;
}

} // namespace sf1r

#endif // CASSANDRA_ADAPTOR_H
