/**
 * @file t_BrokerAgentConfig.cpp
 * @author Ian Yang
 * @date Created <2010-08-23 11:10:22>
 */
#include <boost/test/unit_test.hpp>
#include <configuration-manager/BrokerAgentConfig.h>

using sf1r::BrokerAgentConfig;

BOOST_AUTO_TEST_SUITE(BrokerAgentConfig_test)

BOOST_AUTO_TEST_CASE(Ctor_test)
{
    BrokerAgentConfig config;
    BOOST_CHECK(!config.useCache());
    BOOST_CHECK(config.threadNums() == 2);
    BOOST_CHECK(config.enableTest());
}

BOOST_AUTO_TEST_CASE(UseCache_test)
{
    BrokerAgentConfig config;

    config.setUseCache(true);
    BOOST_CHECK(config.useCache());

    config.setUseCache(false);
    BOOST_CHECK(!config.useCache());
}

BOOST_AUTO_TEST_CASE(ThreadNums_test)
{
    BrokerAgentConfig config;

    config.setThreadNums(1);
    BOOST_CHECK(1 == config.threadNums());
}

BOOST_AUTO_TEST_CASE(EnableTest_test)
{
    BrokerAgentConfig config;

    config.setEnableTest(false);
    BOOST_CHECK(!config.enableTest());

    config.setEnableTest(true);
    BOOST_CHECK(config.enableTest());
}

BOOST_AUTO_TEST_SUITE_END() // BrokerAgentConfig_test
