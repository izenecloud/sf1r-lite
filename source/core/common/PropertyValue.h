#ifndef SF1V5_DOCUMENT_MANAGER_PROPERTY_VALUE_H
#define SF1V5_DOCUMENT_MANAGER_PROPERTY_VALUE_H
/**
 * @file sf1r/document-manager/PropertyValue.h
 * @date Created <2009-10-21 18:15:33>
 * @date Updated <2009-10-22 03:46:01>
 */
#include "type_defs.h"

#include <util/ustring/UString.h>

#include <util/izene_serialization.h>

#include <boost/variant.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/pop_front.hpp>

#include <3rdparty/msgpack/msgpack/object.hpp>
#include <3rdparty/msgpack/msgpack/type/int.hpp>
#include <3rdparty/msgpack/msgpack/type/float.hpp>
#include <3rdparty/msgpack/msgpack/type/string.hpp>
#include <3rdparty/msgpack/msgpack/type/UString.hpp>
#include <net/aggregator/Util.h>

#include <string>
#include <vector>
#include <typeinfo>
#include <ostream>

namespace sf1r {
class PropertyValue;
} // namespace sf1r
MAKE_FEBIRD_SERIALIZATION(sf1r::PropertyValue)

namespace sf1r {

class PropertyValue {
public:
/*****************************************
* boost::variant will construct a default value according
* to the first type. Since izenelib::util::UString is used as
* default, we should make it locate at first position
******************************************/
    typedef boost::variant<
        izenelib::util::UString,
        std::string,
        int32_t,
        int64_t,
        float,
        double,
        std::vector<izenelib::util::UString>,
        std::vector<uint32_t>
    > variant_type;

    typedef variant_type::types type_list;
    typedef std::string  PropertyValueStrType;

    PropertyValue()
    : data_()
    {}

    template<typename T>
    explicit PropertyValue(const T& data)
    : data_(data)
    {}

    template<typename T>
    PropertyValue& operator=(const T& data)
    {
        data_ = data;
        return *this;
    }

    template<typename T>
    PropertyValue& assign(const T& data)
    {
        return this->operator=(data);
    }

    template<typename T>
    T& get()
    {
        return boost::get<T>(data_);
    }
    template<typename T>
    const T& get() const
    {
        return boost::get<T>(data_);
    }
    template<typename T>
    T* getPointer()
    {
        return boost::get<T>(&data_);
    }
    template<typename T>
    const T* getPointer() const
    {
        return boost::get<T>(&data_);
    }

    const std::type_info& type() const
    {
        return data_.type();
    }
    int which() const
    {
        return data_.which();
    }

    void swap(PropertyValue& rhs)
    {
        data_.swap(rhs.data_);
    }

    variant_type& getVariant()
    {
        return data_;
    }
    const variant_type& getVariant() const
    {
        return data_;
    }

    const PropertyValueStrType& getPropertyStrValue() const
    {
        return boost::get<PropertyValueStrType>(data_);
    }
    PropertyValueStrType& getPropertyStrValue()
    {
        return boost::get<PropertyValueStrType>(data_);
    }

    double getNumber() const
    {
        // get without consider the accuracy loss.
        if(data_.type() == typeid(int32_t))
            return boost::get<int32_t>(data_);
        else if(data_.type() == typeid(int64_t))
            return boost::get<int64_t>(data_);
        else if(data_.type() == typeid(float))
            return boost::get<float>(data_);
        else
            return boost::get<double>(data_);
    }

private:
    variant_type data_;
};

// the interface for the PropertyValue with std::string internal string type.
//
inline const std::string& propstr_to_str(
    const PropertyValue::PropertyValueStrType& propstr,
    izenelib::util::UString::EncodingType encode = izenelib::util::UString::UTF_8)
{
    return propstr;
}

inline izenelib::util::UString propstr_to_ustr(
    const PropertyValue::PropertyValueStrType& propstr,
    izenelib::util::UString::EncodingType encode = izenelib::util::UString::UTF_8)
{
    return izenelib::util::UString(propstr, encode);
}

inline const PropertyValue::PropertyValueStrType& str_to_propstr(const std::string& prop_str,
    izenelib::util::UString::EncodingType encode = izenelib::util::UString::UTF_8)
{
    return prop_str;
}

inline PropertyValue::PropertyValueStrType ustr_to_propstr(
    const izenelib::util::UString& prop_ustr,
    izenelib::util::UString::EncodingType encode = izenelib::util::UString::UTF_8)
{
    std::string prop_str;
    prop_ustr.convertString(prop_str, encode);
    return prop_str;
}
// the interface for the PropertyValue with izenelib::util::UString internal string type.
//
//inline std::string propstr_to_str(
//    const PropertyValue::PropertyValueStrType& propstr,
//    izenelib::util::UString::EncodingType encode = izenelib::util::UString::UTF_8)
//{
//    std::string tmpstr;
//    propstr.convertString(tmpstr, encode);
//    return tmpstr;
//}
//
//inline const izenelib::util::UString& propstr_to_ustr(
//    const PropertyValue::PropertyValueStrType& propstr,
//    izenelib::util::UString::EncodingType encode = izenelib::util::UString::UTF_8)
//{
//    return propstr;
//}
//
//inline PropertyValue::PropertyValueStrType str_to_propstr(const std::string& prop_str,
//    izenelib::util::UString::EncodingType encode = izenelib::util::UString::UTF_8)
//{
//    return izenelib::util::UString(prop_str, encode);
//}
//
//inline const PropertyValue::PropertyValueStrType& ustr_to_propstr(
//    const izenelib::util::UString& prop_ustr,
//    izenelib::util::UString::EncodingType encode = izenelib::util::UString::UTF_8)
//{
//    return prop_ustr;
//}

template<typename T>
inline T& get(PropertyValue& p)
{
    return p.get<T>();
}
template<typename T>
inline const T& get(const PropertyValue& p)
{
    return p.get<T>();
}
template<typename T>
inline T* get(PropertyValue* p)
{
    if (p)
    {
        return p->getPointer<T>();
    }

    return 0;
}
template<typename T>
inline const T* get(const PropertyValue* p)
{
    if (p)
    {
        return p->getPointer<T>();
    }

    return 0;
}

inline void swap(PropertyValue& a, PropertyValue& b)
{
    a.swap(b);
}

inline bool operator==(const PropertyValue& a,
                       const PropertyValue& b)
{
    return a.getVariant() == b.getVariant();
}
inline bool operator!=(const PropertyValue& a,
                       const PropertyValue& b)
{
    return !(a == b);
}

template<typename E, typename T>
struct PropertyValuePrinter
: boost::static_visitor<>
{
    PropertyValuePrinter(std::basic_ostream<E, T>& os)
    : os_(os)
    {
    }

    template<typename V>
    void operator()(const V& value)
    {
        os_ << value;
    }

    template<typename V>
    void operator()(const std::vector<V>&)
    {
        os_ << "Vector Type";
    }

private:
    std::basic_ostream<E, T>& os_;
};

template<typename E, typename T>
inline std::basic_ostream<E, T>&
operator<<(std::basic_ostream<E, T>& os,
           const PropertyValue& p)
{
    PropertyValuePrinter<E, T> printer(os);
    boost::apply_visitor(printer, p.getVariant());

    return os;
}

//////////////////////////////////////////////////
// Febird Serialization

template<class DataIO>
struct DataIOSaveVisitor : boost::static_visitor<> {
    DataIOSaveVisitor(DataIO& io)
    : io_(io)
    {}

    template<class T>
    void operator()(const T& value) const
    {
        io_ & value;
    }
private:
    DataIO& io_;
};

template<class DataIO>
void DataIO_saveObject(DataIO& ar, const PropertyValue& p)
{
    int which = p.which();
    ar & which;
    boost::apply_visitor(DataIOSaveVisitor<DataIO>(ar), p.getVariant());
}

template<class Types, class Enable = void>
struct DataIO_propertyValueLoader
{
    template<typename DataIO>
    static void load(DataIO& ar, int which, PropertyValue& p)
    {
        if (which == 0) {
            typedef typename boost::mpl::front<Types>::type head_type;
            head_type value;
            ar & value;
            p = value;
            return;
        }
        typedef typename boost::mpl::pop_front<Types>::type type;
        DataIO_propertyValueLoader<type>::load(ar, which - 1, p);
    }
};

template<class Types>
struct DataIO_propertyValueLoader<
    Types,
    typename boost::enable_if<boost::mpl::empty<Types> >::type
>
{
    template<typename DataIO>
    static void load(DataIO&, int, PropertyValue&)
    {}
};

template<class DataIO>
void DataIO_loadObject(DataIO& ar, PropertyValue& p) {
    int which(0);
    ar & which;

    if (which >= boost::mpl::size<PropertyValue::type_list>::value)
    {
        throw std::runtime_error(
            "Failed to load data"
        );
    }

    DataIO_propertyValueLoader<PropertyValue::type_list>::load(ar, which, p);
}

} // end namespace sf1r

//////////////////////////////////////////////////
// Boost Serialization

namespace boost { namespace serialization {

template<class Archive>
void serialize(Archive & ar,
               sf1r::PropertyValue& p,
               const unsigned int version)
{
    ar & p.getVariant();
}

}} // namespace boost::serialization

///////////////////////////////////////////////////
// Msgpack Serialization

namespace msgpack
{
static const char STR_TYPE_STRING = 's';
static const char STR_TYPE_USTRING = 'u';

inline sf1r::PropertyValue& operator>>(object o, sf1r::PropertyValue& v)
{
    if(o.type == type::POSITIVE_INTEGER)
    {
        v = type::detail::convert_integer<int64_t>(o); return v;
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
    if (int32_t* p = boost::get<int32_t>(&v.getVariant()))
    {
        o.pack_fix_int32(*p);
    }
    else if (int64_t* p = boost::get<int64_t>(&v.getVariant()))
    {
        o.pack_fix_int64(*p);
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
    if (int32_t* p = boost::get<int32_t>(&v.getVariant()))
    {
        *p < 0 ? (o.type = type::NEGATIVE_INTEGER, o.via.i64 = *p, o.raw_type = type::RAW_INT64)
               : (o.type = type::POSITIVE_INTEGER, o.via.u64 = *p);
    }
    else if (int64_t* p = boost::get<int64_t>(&v.getVariant()))
    {
        *p < 0 ? (o.type = type::NEGATIVE_INTEGER, o.via.i64 = *p, o.raw_type = type::RAW_INT64)
               : (o.type = type::POSITIVE_INTEGER, o.via.u64 = *p);
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

} // namespace msgpack

#endif // SF1V5_DOCUMENT_MANAGER_PROPERTY_VALUE_H
