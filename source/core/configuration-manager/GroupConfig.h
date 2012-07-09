#ifndef SF1R_GROUP_CONFIG_H_
#define SF1R_GROUP_CONFIG_H_

#include <common/type_defs.h> // PropertyDataType

#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The type of group property configuration.
 */
class GroupConfig
{
public:
    /// property name
    std::string propName;

    /// property type
    PropertyDataType propType;

    GroupConfig()
        : propType(UNKNOWN_DATA_PROPERTY_TYPE)
    {}

    GroupConfig(const std::string& name, PropertyDataType type)
        : propName(name)
        , propType(type)
    {}

    bool isStringType() const
    {
        return propType == STRING_PROPERTY_TYPE;
    }

    bool isNumericType() const
    {
        return (propType == INT32_PROPERTY_TYPE ||
                propType == FLOAT_PROPERTY_TYPE ||
                propType == INT64_PROPERTY_TYPE ||
                propType == DOUBLE_PROPERTY_TYPE);
    }

    bool isDateTimeType() const
    {
        return propType == DATETIME_PROPERTY_TYPE;
    }

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & propName;
    }
};

} // namespace

#endif //_GROUP_CONFIG_H_
