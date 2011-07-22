///
/// @File   ConfigurationTool.cpp
/// @brief  Source file of configuration tool
/// @author Dohyun Yun
/// @date   2009.10.21
///

#include "ConfigurationTool.h"
namespace sf1r
{
namespace config_tool
{

bool buildPropertyAliasMap( const std::set<PropertyConfig , PropertyComp>& propertyList,
                            PROPERTY_ALIAS_MAP_T&  propertyAliasMap )
{
    propertyAliasMap.clear();

    std::set<PropertyConfig, PropertyComp>::const_iterator propertyIter;
    for (propertyIter = propertyList.begin(); propertyIter != propertyList.end(); propertyIter++)
    {
        std::string originalName( propertyIter->getOriginalName() );
        if ( originalName.empty() || originalName == propertyIter->getName() )
            continue;

        PROPERTY_ALIAS_MAP_T::iterator propertyMapIter
        = propertyAliasMap.find( originalName );
        if ( propertyMapIter == propertyAliasMap.end() )
        {
            std::vector<PropertyConfig> propertyVector;
            propertyVector.push_back( *propertyIter );

            propertyAliasMap.insert( make_pair( originalName , propertyVector ) );
        } // end - else
        else
        {
            propertyMapIter->second.push_back( *propertyIter );
        } // end - else

    } // end - for

    return true;
} // end - buildPropertyAliasMap()


} // end - namespace config-tool
} // end - namespace sf1r
