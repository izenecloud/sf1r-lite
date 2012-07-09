#ifndef SF1R_COMMON_UTILITY_H
#define SF1R_COMMON_UTILITY_H

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
    static time_t createTimeStampInSeconds(const izenelib::util::UString& dataStr, const izenelib::util::UString::EncodingType& encoding, izenelib::util::UString& outDateStr);
    static time_t createTimeStampInSeconds(const izenelib::util::UString& text);
    static time_t createTimeStampInSeconds(const std::string& text);
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

    //static void uint128ToUuid(const uint128_t& val, std::string& str);
    //static void uuidToUint128(const std::string& str, uint128_t& val);
    //static void uuidToUint128(const izenelib::util::UString& ustr, uint128_t& val);

    //static void uint128ToMD5(const uint128_t& val, std::string& str);
    //static void md5ToUint128(const std::string& str, uint128_t& val);

    static std::string uint128ToUuid(const uint128_t& val);
    static uint128_t uuidToUint128(const std::string& str);
    static uint128_t uuidToUint128(const izenelib::util::UString& ustr);

    static std::string uint128ToMD5(const uint128_t& val);
    static uint128_t md5ToUint128(const std::string& str);
    static uint128_t md5ToUint128(const izenelib::util::UString& ustr);

    template <typename AssocType, typename SeqType>
    static void getKeyList(SeqType& key_list, const AssocType& src_map);
    template <typename AssocType, typename SeqType>
    static void getValueList(SeqType& value_list, const AssocType& src_map);

    static bool convertPropertyDataType(const std::string& property_name, const PropertyDataType& sf1r_type, izenelib::ir::indexmanager::PropertyType& type);

    /**
     * Round up @p x to multiple of @p base.
     */
    static std::size_t roundUp(std::size_t x, std::size_t base)
    {
        if (base == 0)
            return x;

        return (x + base - 1) / base * base;
    }

    /**
     * Round down @p x to multiple of @p base.
     */
    static std::size_t roundDown(std::size_t x, std::size_t base)
    {
        if (base == 0)
            return x;

        return x / base * base;
    }
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
