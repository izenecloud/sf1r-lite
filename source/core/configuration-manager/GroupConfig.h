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
