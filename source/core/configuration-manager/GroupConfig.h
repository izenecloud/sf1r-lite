#ifndef SF1R_GROUP_CONFIG_H_
#define SF1R_GROUP_CONFIG_H_

#include <common/type_defs.h> // PropertyDataType

#include <string>
#include <map>
#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The group property configuration.
 */
class GroupConfig
{
public:
    /// property type
    PropertyDataType propType;

    /// true when configured like this:
    /// <Group><Property ... rebuild="y"/>
    bool isConfigAsRebuild;

    /// true when configured like this:
    /// <IndexBundle><Schema><Indexing ... rtype="y"/>
    bool isRTypeStr;

    GroupConfig()
        : propType(UNKNOWN_DATA_PROPERTY_TYPE)
        , isConfigAsRebuild(false)
        , isRTypeStr(false)
    {}

    GroupConfig(PropertyDataType type)
        : propType(type)
        , isConfigAsRebuild(false)
        , isRTypeStr(false)
    {}

    bool isStringType() const
    {
        return propType == STRING_PROPERTY_TYPE;
    }

    bool isNumericType() const
    {
        return propType == INT32_PROPERTY_TYPE ||
               propType == FLOAT_PROPERTY_TYPE ||
               propType == INT8_PROPERTY_TYPE ||
               propType == INT16_PROPERTY_TYPE ||
               propType == INT64_PROPERTY_TYPE ||
               propType == DOUBLE_PROPERTY_TYPE;
    }

    bool isDateTimeType() const
    {
        return propType == DATETIME_PROPERTY_TYPE;
    }

    /// whether need to rebuild when each time
    /// GroupManager::processCollection() is called
    bool isRebuild() const
    {
        return isConfigAsRebuild || isRTypeStr;
    }

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & propType;
        ar & isRebuild;
    }
};

/// key: property name
typedef std::map<std::string, GroupConfig> GroupConfigMap;

} // namespace

#endif //_GROUP_CONFIG_H_
