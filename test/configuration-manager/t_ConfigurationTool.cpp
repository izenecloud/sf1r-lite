/**
 * @file t_ConfigurationTool.cpp
 * @author Ian Yang
 * @date Created <2010-08-24 11:49:03>
 */
#include <boost/test/unit_test.hpp>
#include <configuration-manager/ConfigurationTool.h>

using namespace sf1r;

namespace { // {anonymous}
class ConfigurationToolFixture
{
public:
    IndexBundleSchema propertyList;
    config_tool::PROPERTY_ALIAS_MAP_T aliasMap;
};
} // namespace {anonymous}


BOOST_FIXTURE_TEST_SUITE(ConfigurationTool_test, ConfigurationToolFixture)

BOOST_AUTO_TEST_CASE(OriginalPropertyIsNotAddedInAliasMap_test)
{
    PropertyConfig original;
    original.setName("original");
    original.setOriginalName("original");

    propertyList.insert(original);

    config_tool::buildPropertyAliasMap(propertyList, aliasMap);

    BOOST_CHECK(aliasMap.empty());
}

BOOST_AUTO_TEST_SUITE_END() // ConfigurationTool_test
