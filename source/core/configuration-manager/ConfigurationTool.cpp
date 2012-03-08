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

bool buildPropertyAliasMap( const IndexBundleSchema& propertyList,
                            PROPERTY_ALIAS_MAP_T&  propertyAliasMap )
{
    propertyAliasMap.clear();

    for (IndexBundleSchema::const_iterator propertyIter = propertyList.begin();
        propertyIter != propertyList.end(); ++propertyIter)
    {
        if (!propertyIter->isAliasProperty())
            continue;

        std::string originalName( propertyIter->getOriginalName() );
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
