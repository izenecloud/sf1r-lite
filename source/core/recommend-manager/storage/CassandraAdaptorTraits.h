/**
 * @file CassandraAdaptorTraits.h
 * @brief utility functions used in CassandraAdaptor
 * @author Jun Jiang
 * @date 2012-02-02
 */

#ifndef CASSANDRA_ADAPTOR_TRAITS_H
#define CASSANDRA_ADAPTOR_TRAITS_H

#include <string>
#include <set>
#include <boost/lexical_cast.hpp>

namespace sf1r
{

struct CassandraAdaptorTraits
{
    template <class ContainerType, typename ValueType>
    static void insertContainer(ContainerType& container, const ValueType& value)
    {
        container.push_back(value);
    }

    template <typename ValueType>
    static void insertContainer(std::set<ValueType>& container, const ValueType& value)
    {
        container.insert(value);
    }

    template <typename Source>
    static std::string convertToString(const Source& src)
    {
        return boost::lexical_cast<std::string>(src);
    }

    static const std::string& convertToString(const std::string& src)
    {
        return src;
    }

};

} // namespace sf1r

#endif // CASSANDRA_ADAPTOR_TRAITS_H
