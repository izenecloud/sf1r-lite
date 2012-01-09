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

    typedef boost::variant<
        int64_t,
        uint64_t,
        float,
        double,
        std::string,
        izenelib::util::UString,
        std::vector<izenelib::util::UString>,
        std::vector<uint32_t>
    > variant_type;

    typedef variant_type::types type_list;

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


private:
    variant_type data_;
};

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
            head_type value(0);
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

#endif // SF1V5_DOCUMENT_MANAGER_PROPERTY_VALUE_H
