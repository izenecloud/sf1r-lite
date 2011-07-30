/**
 * @file sf1_msgpack_serialization_types.h
 * @author Zhongxia Li
 * @date Jul 28, 2011
 * @brief Make SF1 data types can be serialized by MSGPACK.
 */
#ifndef SF1_MSGPACK_SERIALIZATION_TYPES_H_
#define SF1_MSGPACK_SERIALIZATION_TYPES_H_

#include <3rdparty/msgpack/msgpack/object.hpp>
#include <3rdparty/msgpack/msgpack/type/int.hpp>
#include <3rdparty/msgpack/msgpack/type/float.hpp>
#include <3rdparty/msgpack/msgpack/type/string.hpp>
#include <3rdparty/msgpack/msgpack/type/UString.hpp>

#include <ranking-manager/RankingEnumerator.h>
#include <query-manager/QueryTypeDef.h>
#include <common/PropertyValue.h>
#include <util/ustring/UString.h>

namespace msgpack {
/// RankingType
inline sf1r::RankingType::TextRankingType& operator>>(object o, sf1r::RankingType::TextRankingType& v)
{
    signed int iv = type::detail::convert_integer<signed int>(o);
    v = static_cast<sf1r::RankingType::TextRankingType>(iv);
    return v;
}

template <typename Stream>
inline packer<Stream>& operator<< (packer<Stream>& o, const sf1r::RankingType::TextRankingType& v)
    { o.pack_int(static_cast<int>(v)); return o; }

inline void operator<< (object& o, sf1r::RankingType::TextRankingType v)
    { o.type = type::POSITIVE_INTEGER, o.via.u64 = static_cast<int>(v); }

inline void operator<< (object::with_zone& o, sf1r::RankingType::TextRankingType v)
    { static_cast<object&>(o) << static_cast<int>(v); }


/// QueryFiltering::FilteringOperation
inline sf1r::QueryFiltering::FilteringOperation& operator>>(object o, sf1r::QueryFiltering::FilteringOperation& v)
{
    signed int iv = type::detail::convert_integer<signed int>(o);
    v = static_cast<sf1r::QueryFiltering::FilteringOperation>(iv);
    return v;
}

template <typename Stream>
inline packer<Stream>& operator<< (packer<Stream>& o, const sf1r::QueryFiltering::FilteringOperation& v)
    { o.pack_int(static_cast<int>(v)); return o; }

inline void operator<< (object& o, sf1r::QueryFiltering::FilteringOperation v)
    { o.type = type::POSITIVE_INTEGER, o.via.u64 = static_cast<int>(v); }

inline void operator<< (object::with_zone& o, sf1r::QueryFiltering::FilteringOperation v)
    { static_cast<object&>(o) << static_cast<int>(v); }

/// PropertyValue
static const char STR_TYPE_STRING = 's';
static const char STR_TYPE_USTRING = 'u';

inline sf1r::PropertyValue& operator>>(object o, sf1r::PropertyValue& v)
{
    if(o.type == type::POSITIVE_INTEGER)
    {
        if (o.raw_type == type::RAW_UINT64)
        {
            v = type::detail::convert_integer<uint64_t>(o); return v;
        }
        else
        {
            v = type::detail::convert_integer<int64_t>(o); return v;
        }
    }
    else if(o.type == type::NEGATIVE_INTEGER)
    {
        v = type::detail::convert_integer<int64_t>(o); return v;
    }
    else if (o.type == type::DOUBLE)
    {
        v = (double)o.via.dec;
    }
    else if (o.type == type::RAW)
    {
        std::string s;
        o >> s;

        if (s.length() <= 1)
        {
            v = s;
        }
        else if (s[0] == STR_TYPE_USTRING)
        {
            std::string str = s.substr(1);
            izenelib::util::UString ustr(str, izenelib::util::UString::UTF_8);
            v = ustr;
        }
        else
        {
            std::string str = s.substr(1);
            v = str;
        }
    }

    return v;
}

template <typename Stream>
inline packer<Stream>& operator<< (packer<Stream>& o, const sf1r::PropertyValue& cv)
{
    sf1r::PropertyValue& v = const_cast<sf1r::PropertyValue&>(cv);
    if (int64_t* p = boost::get<int64_t>(&v.getVariant()))
    {
        o.pack_fix_int64(*p);
    }
    else if (uint64_t* p = boost::get<uint64_t>(&v.getVariant()))
    {
        o.pack_fix_uint64(*p);
    }
    else if (const float* p = boost::get<float>(&v.getVariant()))
    {
        o.pack_double(*p);
    }
    else if (double* p = boost::get<double>(&v.getVariant()))
    {
        o.pack_double(*p);
    }
    else if (std::string* p = boost::get<std::string>(&v.getVariant()))
    {
        std::string packstr;
        packstr += STR_TYPE_STRING;
        packstr += *p;
        o.pack_raw(packstr.size());
        o.pack_raw_body(packstr.c_str(), packstr.size());
    }
    else if (izenelib::util::UString* p = boost::get<izenelib::util::UString>(&v.getVariant()))
    {
        std::string packstr;
        packstr += STR_TYPE_USTRING;
        std::string s;
        (*p).convertString(s, izenelib::util::UString::UTF_8);
        packstr += s;
        o.pack_raw(packstr.size());
        o.pack_raw_body(packstr.data(), packstr.size());
    }
    else
    {
        // reserved
        //std::vector<izenelib::util::UString>,
        //std::vector<uint32_t>

        throw type_error();
    }

    return o;
}

inline void operator<< (object& o, sf1r::PropertyValue v)
{
    if (int64_t* p = boost::get<int64_t>(&v.getVariant()))
    {
        *p < 0 ? (o.type = type::NEGATIVE_INTEGER, o.via.i64 = *p, o.raw_type = type::RAW_INT64)
               : (o.type = type::POSITIVE_INTEGER, o.via.u64 = *p);
    }
    else if (uint64_t* p = boost::get<uint64_t>(&v.getVariant()))
    {
        o.type = type::POSITIVE_INTEGER, o.via.u64 = *p,  o.raw_type = type::RAW_UINT64;
    }
    else if (float* p = boost::get<float>(&v.getVariant()))
    {
        o << *p;
    }
    else if (double* p = boost::get<double>(&v.getVariant()))
    {
        o << *p;
    }
    else if (std::string* p = boost::get<std::string>(&v.getVariant()))
    {
        std::string packstr;
        packstr += STR_TYPE_STRING;
        packstr += *p;
        o << packstr;
    }
    else if (izenelib::util::UString* p = boost::get<izenelib::util::UString>(&v.getVariant()))
    {
        std::string packstr;
        packstr += STR_TYPE_USTRING;
        std::string s;
        (*p).convertString(s, izenelib::util::UString::UTF_8);
        packstr += s;
        o << packstr;
    }
    else
        throw type_error();
}

inline void operator<< (object::with_zone& o, sf1r::PropertyValue v)
{
    if (int64_t* p = boost::get<int64_t>(&v.getVariant()))
    {
        static_cast<object&>(o) << *p;
    }
    else if (uint64_t* p = boost::get<uint64_t>(&v.getVariant()))
    {
        static_cast<object&>(o) << *p;
    }
    else if (float* p = boost::get<float>(&v.getVariant()))
    {
        static_cast<object&>(o) << *p;
    }
    else if (double* p = boost::get<double>(&v.getVariant()))
    {
        static_cast<object&>(o) << *p;
    }
    else if (std::string* p = boost::get<std::string>(&v.getVariant()))
    {
        std::string packstr;
        packstr += STR_TYPE_STRING;
        packstr += *p;
        static_cast<object&>(o) << packstr;
    }
    else if (izenelib::util::UString* p = boost::get<izenelib::util::UString>(&v.getVariant()))
    {
        std::string packstr;
        packstr += STR_TYPE_USTRING;
        std::string s;
        (*p).convertString(s, izenelib::util::UString::UTF_8);
        packstr += s;
        static_cast<object&>(o) << packstr;
    }
    else
        throw type_error();
}

} // namespace


#endif /* SF1_MSGPACK_SERIALIZATION_TYPES_H_ */
