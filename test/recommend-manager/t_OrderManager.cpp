/// @file t_OrderManager.cpp
/// @brief test OrderManager

#include "OrderManagerTestFixture.h"
#include <recommend-manager/storage/OrderManager.h>

#include <boost/test/unit_test.hpp>

using namespace sf1r;

namespace
{
const char* ORDER_HOME_STR = "recommend_test/t_OrderManager";
}

BOOST_FIXTURE_TEST_SUITE(OrderManagerTest, OrderManagerTestFixture)

BOOST_AUTO_TEST_CASE(checkGetFreqSets)
{
    setTestDir(ORDER_HOME_STR);
    resetInstance();

    addOrder("1 2 3 4");
    addOrder("2 3");
    addOrder("3 4 5");

    checkFreqItems("1 2 3", "4");
    checkFreqItems("1 2", "3 4");
    checkFreqItems("2 3 4", "1");
    checkFreqItems("3 4", "1 2 5");
    checkFreqItems("4 5", "3");

    checkFreqBundles();

    resetInstance();
    checkFreqItems("1", "2 3 4");
    checkFreqItems("2", "1 3 4");
    checkFreqItems("3", "1 2 4 5");
    checkFreqItems("4", "1 2 3 5");
    checkFreqItems("5", "3 4");
}

BOOST_AUTO_TEST_CASE(checkAllFreqSets)
{
    setTestDir(ORDER_HOME_STR);
    resetInstance();

    for (int i=0; i<1000; ++i)
    {
        addOrder("1");
    }

    for (int i=0; i<5000; ++i)
    {
        addOrder("2");
    }

    addOrder("2 3");

    checkFreqBundles();
}

BOOST_AUTO_TEST_SUITE_END() 

