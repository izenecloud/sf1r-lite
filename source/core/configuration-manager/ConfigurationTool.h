///
/// @File   ConfigurationTool.h
/// @brief  Header file of configuration tool
/// @author Dohyun Yun
/// @date   2009.10.21
///

#ifndef _CONFIGURATIONTOOL_H_
#define _CONFIGURATIONTOOL_H_

#include "PropertyConfig.h"
#include <boost/unordered_map.hpp>


namespace sf1r
{
namespace config_tool
{

///
/// @brief  Defines property alias type.
///         Key is Original Property name and Value is the vector of property name.
///
typedef boost::unordered_map<
std::string,
std::vector<PropertyConfig>
> PROPERTY_ALIAS_MAP_T;

///
/// @brief Builds property alias map by using property Config set.
/// @param propertyList Property set which stores information of property name.
/// @param propertyAliasMap Result data of this function.
/// @return true    Success to build property alias map.
/// @return false   Fail to build property alias map.
///
bool buildPropertyAliasMap( const IndexBundleSchema& propertyList,
                            PROPERTY_ALIAS_MAP_T&  propertyAliasMap );

} // end - namespace config_tool
} // end - namespace sf1r

#endif // _CONFIGURATIONTOOL_H_
