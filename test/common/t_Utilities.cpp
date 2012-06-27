///
/// @file t_Utilities.cpp
/// @brief test utility functions
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-04-26
///

#include <common/Utilities.h>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <vector>

using namespace boost::gregorian;
using namespace boost::posix_time;

using namespace sf1r;
using izenelib::util::UString;

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

BOOST_AUTO_TEST_CASE(testTimestamp)
{
    std::string datestr1("20120627093011");
    int64_t timestamp1 = Utilities::createTimeStampInSeconds(datestr1);
    std::string datestr2("20120627253011");
    int64_t timestamp2 = Utilities::createTimeStampInSeconds(datestr2);
    BOOST_CHECK(timestamp1<timestamp2);
    std::string datestr1_1("2012-06-27 09:30:11");
    timestamp2 = Utilities::createTimeStampInSeconds(datestr1_1);
    BOOST_CHECK_EQUAL(timestamp1, timestamp2);

    std::string datestr3("2012-06-27 16:12:21");
    timestamp1 = Utilities::createTimeStampInSeconds(datestr3);
    std::string datestr4("2012-06-27 23:12:21");
    timestamp2 = Utilities::createTimeStampInSeconds(datestr4);
    BOOST_CHECK(timestamp1<timestamp2);
}
BOOST_AUTO_TEST_SUITE_END() 
