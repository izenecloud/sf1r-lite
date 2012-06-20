///
/// @file t_GroupLabelLogger.cpp
/// @brief test getting frequent group labels
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-26
///

#include "GroupLabelLoggerTestFixture.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(GroupLabelLoggerTest, sf1r::GroupLabelLoggerTestFixture)

BOOST_AUTO_TEST_CASE(testLogLabel)
{
    setTestData(100, 10); // query range, label range
    const int LOG_NUM = 10000; // log number

    resetInstance();

    BOOST_TEST_MESSAGE("check empty log");
    checkLabel();

    BOOST_TEST_MESSAGE("create log 1st time");
    createLog(LOG_NUM);
    checkLabel();

    resetInstance();

    BOOST_TEST_MESSAGE("check loaded log");
    checkLabel();

    BOOST_TEST_MESSAGE("create log 2nd time");
    createLog(LOG_NUM);
    checkLabel();
}

BOOST_AUTO_TEST_CASE(testSetLabel)
{
    setTestData(50, 7); // query range, label range
    const int LOG_NUM = 3000; // log number
    const int TOP_NUM = 500; // set top label number

    resetInstance();

    BOOST_TEST_MESSAGE("set top label");
    setLabel(TOP_NUM);
    checkLabel();

    BOOST_TEST_MESSAGE("create log");
    createLog(LOG_NUM);
    setLabel(TOP_NUM);
    checkLabel();

    resetInstance();

    BOOST_TEST_MESSAGE("check loaded log");
    checkLabel();

    BOOST_TEST_MESSAGE("create log 2nd time");
    setLabel(TOP_NUM);
    createLog(LOG_NUM);
    checkLabel();
}

BOOST_AUTO_TEST_SUITE_END() 
