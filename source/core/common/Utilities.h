#ifndef UTILITY_H
#define UTILITY_H

#include "type_defs.h"
#include <ir/index_manager/index/IndexerPropertyConfig.h>
#include <util/ustring/UString.h>

#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;

namespace sf1r{

class Utilities
{
public:
    Utilities() {}
    ~Utilities() {}

public:
    static int64_t convertDate(const izenelib::util::UString& dataStr, const izenelib::util::UString::EncodingType& encoding, izenelib::util::UString& outDateStr);
    static int64_t convertDate(const std::string& dataStr);
    static std::string toLowerCopy(const std::string& str);
    static std::string toUpperCopy(const std::string& str);
    static void toLower(std::string& str);
    static void toUpper(std::string& str);

    static time_t createTimeStamp();
    static time_t createTimeStamp(boost::posix_time::ptime pt);
    static time_t createTimeStamp(const izenelib::util::UString& text);
    static time_t createTimeStamp(const std::string& text);

    template <typename T>
    static std::string toBytes(const T& val)
    {
        return std::string(reinterpret_cast<const char *>(&val), sizeof(T));
    }

    template <typename T>
    static T fromBytes(const std::string& str)
    {
        return *(reinterpret_cast<const T *>(str.c_str()));
    }

    template <typename AssocType, typename SeqType>
    static void getKeyList(SeqType& key_list, const AssocType& src_map)
    {
        for (typename AssocType::const_iterator it = src_map.begin();
                it != src_map.end(); ++it)
        {
            key_list.push_back(it->first);
        }
    }

    template <typename AssocType, typename SeqType>
    static void getValueList(SeqType& value_list, const AssocType& src_map)
    {
        for (typename AssocType::const_iterator it = src_map.begin();
                it != src_map.end(); ++it)
        {
            value_list.push_back(it->second);
        }
    }

    template <std::size_t Index, typename TupleType>
    static bool tupleLess(const TupleType& t1, const TupleType& t2)
    {
        return t1.get<Index>() < t2.get<Index>();
    }

    template <std::size_t Index, typename TupleType>
    static bool tupleGreater(const TupleType& t1, const TupleType& t2)
    {
        return t1.get<Index>() > t2.get<Index>();
    }

    template <std::size_t Index, typename TupleType>
    static bool tupleLessEqual(const TupleType& t1, const TupleType& t2)
    {
        return t1.get<Index>() <= t2.get<Index>();
    }

    template <std::size_t Index, typename TupleType>
    static bool tupleGreaterEqual(const TupleType& t1, const TupleType& t2)
    {
        return t1.get<Index>() >= t2.get<Index>();
    }

    template <std::size_t Index, typename TupleType>
    static bool tupleEqual(const TupleType& t1, const TupleType& t2)
    {
        return t1.get<Index>() == t2.get<Index>();
    }

    template <std::size_t Index, typename TupleType>
    static bool tupleNotEqual(const TupleType& t1, const TupleType& t2)
    {
        return t1.get<Index>() != t2.get<Index>();
    }

    static bool convertPropertyDataType(const std::string& property_name, const PropertyDataType& sf1r_type, izenelib::ir::indexmanager::PropertyType& type);

};

}

#endif
