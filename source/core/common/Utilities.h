#ifndef UTILITY_H
#define UTILITY_H

#include "type_defs.h"
#include <ir/index_manager/index/IndexerPropertyConfig.h>
#include <util/ustring/UString.h>

namespace boost
{
namespace posix_time
{
class ptime;
}
}

namespace sf1r
{

class Utilities
{
public:
    Utilities() {}
    ~Utilities() {}

public:
    static int64_t convertDate(const izenelib::util::UString& dataStr, const izenelib::util::UString::EncodingType& encoding, izenelib::util::UString& outDateStr);
    static int64_t convertDate(const std::string& dataStr);

    static time_t createTimeStamp();
    static time_t createTimeStamp(const boost::posix_time::ptime& pt);
    static time_t createTimeStamp(const izenelib::util::UString& text);
    static time_t createTimeStamp(const std::string& text);

    template <typename T>
    static inline std::string toBytes(const T& val)
    {
        return std::string(reinterpret_cast<const char *>(&val), sizeof(T));
    }
    template <typename T>
    static inline T fromBytes(const std::string& str)
    {
        if (str.length() < sizeof(T))
            return T();

        return *(reinterpret_cast<const T *>(str.c_str()));
    }

    static void uint128ToUuid(const uint128_t& val, std::string& str);
    static void uuidToUint128(const std::string& str, uint128_t& val);

    static void uint128ToMD5(const uint128_t& val, std::string& str);
    static void md5ToUint128(const std::string& str, uint128_t& val);

    static std::string uint128ToUuid(const uint128_t& val);
    static uint128_t uuidToUint128(const std::string& str);

    static std::string uint128ToMD5(const uint128_t& val);
    static uint128_t md5ToUint128(const std::string& str);

    template <typename AssocType, typename SeqType>
    static void getKeyList(SeqType& key_list, const AssocType& src_map);
    template <typename AssocType, typename SeqType>
    static void getValueList(SeqType& value_list, const AssocType& src_map);

    static bool convertPropertyDataType(const std::string& property_name, const PropertyDataType& sf1r_type, izenelib::ir::indexmanager::PropertyType& type);

    static uint32_t calcHammDist(uint64_t v1, uint64_t v2);
};

template <typename AssocType, typename SeqType>
void Utilities::getKeyList(SeqType& key_list, const AssocType& src_map)
{
    key_list.reserve(key_list.size() + src_map.size());
    for (typename AssocType::const_iterator it = src_map.begin();
            it != src_map.end(); ++it)
    {
        key_list.push_back(it->first);
    }
}

template <typename AssocType, typename SeqType>
void Utilities::getValueList(SeqType& value_list, const AssocType& src_map)
{
    value_list.reserve(value_list.size() + src_map.size());
    for (typename AssocType::const_iterator it = src_map.begin();
            it != src_map.end(); ++it)
    {
        value_list.push_back(it->second);
    }
}

}

#endif
