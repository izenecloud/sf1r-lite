///
/// @file t_Utilities.cpp
/// @brief test utility functions
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-04-26
///

#include <common/Utilities.h>
#include <boost/test/unit_test.hpp>

using namespace sf1r;

BOOST_AUTO_TEST_SUITE(Utilities_test)

BOOST_AUTO_TEST_CASE(testRoundUp)
{
    // base 100
    BOOST_CHECK_EQUAL(Utilities::roundUp(0U, 100U), 0U);
    BOOST_CHECK_EQUAL(Utilities::roundUp(1U, 100U), 100U);
    BOOST_CHECK_EQUAL(Utilities::roundUp(33U, 100U), 100U);
    BOOST_CHECK_EQUAL(Utilities::roundUp(99U, 100U), 100U);
    BOOST_CHECK_EQUAL(Utilities::roundUp(100U, 100U), 100U);
    BOOST_CHECK_EQUAL(Utilities::roundUp(101U, 100U), 200U);
    BOOST_CHECK_EQUAL(Utilities::roundUp(124U, 100U), 200U);
    BOOST_CHECK_EQUAL(Utilities::roundUp(1000U, 100U), 1000U);
    BOOST_CHECK_EQUAL(Utilities::roundUp(10003U, 100U), 10100U);

    // base 1
    BOOST_CHECK_EQUAL(Utilities::roundUp(0U, 1U), 0U);
    BOOST_CHECK_EQUAL(Utilities::roundUp(1U, 1U), 1U);
    BOOST_CHECK_EQUAL(Utilities::roundUp(10U, 1U), 10U);
    BOOST_CHECK_EQUAL(Utilities::roundUp(123U, 1U), 123U);

    // base 0
    BOOST_CHECK_EQUAL(Utilities::roundUp(0U, 0U), 0U);
    BOOST_CHECK_EQUAL(Utilities::roundUp(10U, 0U), 10U);
    BOOST_CHECK_EQUAL(Utilities::roundUp(123U, 0U), 123U);
}

BOOST_AUTO_TEST_CASE(testRoundDown)
{
    // base 100
    BOOST_CHECK_EQUAL(Utilities::roundDown(0U, 100U), 0U);
    BOOST_CHECK_EQUAL(Utilities::roundDown(1U, 100U), 0U);
    BOOST_CHECK_EQUAL(Utilities::roundDown(33U, 100U), 0U);
    BOOST_CHECK_EQUAL(Utilities::roundDown(99U, 100U), 0U);
    BOOST_CHECK_EQUAL(Utilities::roundDown(100U, 100U), 100U);
    BOOST_CHECK_EQUAL(Utilities::roundDown(101U, 100U), 100U);
    BOOST_CHECK_EQUAL(Utilities::roundDown(124U, 100U), 100U);
    BOOST_CHECK_EQUAL(Utilities::roundDown(1000U, 100U), 1000U);
    BOOST_CHECK_EQUAL(Utilities::roundDown(10003U, 100U), 10000U);

    // base 1
    BOOST_CHECK_EQUAL(Utilities::roundDown(0U, 1U), 0U);
    BOOST_CHECK_EQUAL(Utilities::roundDown(1U, 1U), 1U);
    BOOST_CHECK_EQUAL(Utilities::roundDown(10U, 1U), 10U);
    BOOST_CHECK_EQUAL(Utilities::roundDown(123U, 1U), 123U);

    // base 0
    BOOST_CHECK_EQUAL(Utilities::roundDown(0U, 0U), 0U);
    BOOST_CHECK_EQUAL(Utilities::roundDown(10U, 0U), 10U);
    BOOST_CHECK_EQUAL(Utilities::roundDown(123U, 0U), 123U);
}

BOOST_AUTO_TEST_SUITE_END() 
