/**
 * @file core/common/utilities/ValueConverter.cpp
 * @author Ian Yang
 * @date Created <2010-06-11 17:54:46>
 */
#include "ValueConverter.h"
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/contains.hpp>

namespace sf1r {

namespace detail {

class DriverValueToPropertyValueConverter : public boost::static_visitor<>
{
public:
    PropertyValue* propertyValue;

    DriverValueToPropertyValueConverter(PropertyValue& v)
    : propertyValue(&v)
    {}

    void operator()(const Value::NullType&) const
    {
        *propertyValue = PropertyValue();
    }

    void operator()(const Value::IntType& val) const
    {
        *propertyValue = (int64_t)val;
    }

    void operator()(const Value::UintType& val) const
    {
        *propertyValue = (uint64_t)val;
    }

    void operator()(const Value::DoubleType& val) const
    {
        *propertyValue = val;
    }

    void operator()(const Value::StringType& val) const
    {
        *propertyValue = val;
    }

    void operator()(const Value::BoolType& array) const
    {
        *propertyValue = (int64_t)0;
    }

    void operator()(const Value::ArrayType& array) const
    {
        *propertyValue = std::string();
    }

    void operator()(const Value::ObjectType& array) const
    {
        *propertyValue = std::string();
    }
};


} // namespace detail

void ValueConverter::driverValue2PropertyValue(
    const driver::Value& driverValue,
    PropertyValue& propertyValue
)
{
    detail::DriverValueToPropertyValueConverter converter(propertyValue);
    boost::apply_visitor(converter, driverValue.variant());
}

void ValueConverter::driverValue2PropertyValue(
    sf1r::PropertyDataType dataType,
    const driver::Value& driverValue,
    PropertyValue& propertyValue
)
{
    switch (dataType)
    {
    case STRING_PROPERTY_TYPE:
        propertyValue = asString(driverValue);
        break;
    case INT_PROPERTY_TYPE:
        propertyValue = (int64_t)asInt(driverValue);
        break;
    case FLOAT_PROPERTY_TYPE:
        propertyValue = (float)asDouble(driverValue);
        break;
    case DOUBLE_PROPERTY_TYPE:
        propertyValue = asDouble(driverValue);
        break;
    default:
        throw std::runtime_error(
            "Cannot convert to internal property data type"
        );
    }
}

} // namespace sf1r
