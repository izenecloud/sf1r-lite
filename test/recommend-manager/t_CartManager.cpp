/// @file t_CartManager.cpp
/// @brief test CartManager in shopping cart operations
/// @author Jun Jiang
/// @date Created 2011-08-07
///

#include "CartManagerTestFixture.h"
#include <recommend-manager/CartManager.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <string>

using namespace std;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const string TEST_DIR_STR = "recommend_test/t_CartManager";
const string CART_DB_STR = "cart.db";
}

void testCart1(
    CartManager& cartManager,
    CartManagerTestFixture& fixture
)
{
    BOOST_TEST_MESSAGE("1st update cart...");

    fixture.setCartManager(&cartManager);

    fixture.updateCart("1", "2 4 6");
    fixture.updateCart("10", "11");
    fixture.updateCart("100", "102 103");

    fixture.updateRandItem("7", 99);
    fixture.updateRandItem("6", 100);
    fixture.updateRandItem("5", 101);
    fixture.updateRandItem("4", 1234);

    fixture.checkCartManager();
}

void testCart2(
    CartManager& cartManager,
    CartManagerTestFixture& fixture
)
{
    BOOST_TEST_MESSAGE("2nd update cart...");

    fixture.setCartManager(&cartManager);

    fixture.updateCart("1", "1 3 5 7 9");
    fixture.updateCart("10", "");
    fixture.updateCart("1000", "1001 1010 1100");
    fixture.updateCart("10000", "10003 10004 10005 10006");

    fixture.checkCartManager();
}

BOOST_AUTO_TEST_SUITE(CartManagerTest)

BOOST_FIXTURE_TEST_CASE(checkLocalCartManager, CartManagerTestFixture)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

    bfs::path cartPath(bfs::path(TEST_DIR_STR) / CART_DB_STR);

    {
        CartManager cartManager(cartPath.string());
        testCart1(cartManager, *this);
    }

    {
        CartManager cartManager(cartPath.string());
        testCart2(cartManager, *this);
    }
}

BOOST_AUTO_TEST_SUITE_END() 
