///
/// @file t_MerchantScoreManager.cpp
/// @brief test operations on merchant score
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-05-11
///

#include "MerchantScoreManagerTestFixture.h"
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(MerchantScoreManagerTest, sf1r::MerchantScoreManagerTestFixture)

BOOST_AUTO_TEST_CASE(testSet)
{
    BOOST_TEST_MESSAGE("check empty score ...");
    checkEmptyScore();

    BOOST_TEST_MESSAGE("set score 1 ...");
    setScore1();
    checkScore1();

    BOOST_TEST_MESSAGE("reset instance ...");
    resetInstance();
    checkScore1();

    BOOST_TEST_MESSAGE("set score 2 ...");
    setScore2();
    checkScore2();
}

BOOST_AUTO_TEST_SUITE_END()
