/**
 * @file t_PropertyConfig.cpp
 * @author Ian Yang
 * @date Created <2010-08-23 15:02:35>
 */
#include <boost/test/unit_test.hpp>
#include <configuration-manager/PropertyConfig.h>

namespace { // {anonymous}
class PropertyConfigFixture
{
public:
    sf1r::PropertyConfig config;
};
} // namespace {anonymous}

BOOST_FIXTURE_TEST_SUITE(PropertyConfig_test, PropertyConfigFixture)

BOOST_AUTO_TEST_CASE(PropertyId_test)
{
    BOOST_CHECK(config.getPropertyId() == 0);

    config.setPropertyId(1);
    BOOST_CHECK(config.getPropertyId() == 1);
}

BOOST_AUTO_TEST_CASE(Name_test)
{
    BOOST_CHECK(config.getName().empty());

    config.setName("test");
    BOOST_CHECK(config.getName() == "test");
}

BOOST_AUTO_TEST_CASE(OriginalName_test)
{
    BOOST_CHECK(config.getOriginalName().empty());

    config.setOriginalName("test");
    BOOST_CHECK(config.getOriginalName() == "test");
}

BOOST_AUTO_TEST_CASE(Type_test)
{
    BOOST_CHECK(config.getType() == sf1r::UNKNOWN_DATA_PROPERTY_TYPE);

    config.setType(sf1r::INT32_PROPERTY_TYPE);
    BOOST_CHECK(config.getType() == sf1r::INT32_PROPERTY_TYPE);
}

BOOST_AUTO_TEST_CASE(IsIndex_test)
{
    BOOST_CHECK(!config.isIndex());

    config.setIsIndex(true);
    BOOST_CHECK(config.isIndex());

    config.setIsIndex(false);
    BOOST_CHECK(!config.isIndex());
}


BOOST_AUTO_TEST_SUITE_END() // PropertyConfig_test
