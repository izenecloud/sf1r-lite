/// @file t_CartManager.cpp
/// @brief test CartManager in shopping cart operations
/// @author Jun Jiang
/// @date Created 2011-08-07
///

#include "CartManagerTestFixture.h"
#include "test_util.h"
#include <recommend-manager/storage/CartManager.h>

#include <boost/test/unit_test.hpp>
#include <string>

using namespace std;
using namespace sf1r;

namespace
{
const string TEST_DIR_STR = "recommend_test/t_CartManager";
const string KEYSPACE_NAME = "test_recommend";
const string COLLECTION_NAME = "example";

void testCart1(CartManagerTestFixture& fixture)
{
    BOOST_TEST_MESSAGE("1st update cart...");

    fixture.resetInstance();

    fixture.updateCart("1", "2 4 6");
    fixture.updateCart("2", "3 5 3 5");
    fixture.updateCart("10", "11");
    fixture.updateCart("100", "102 103");

    fixture.updateRandItem("7", 99);
    fixture.updateRandItem("6", 100);
    fixture.updateRandItem("5", 101);
    fixture.updateRandItem("4", 1234);

    fixture.checkCartManager();
}

void testCart2(CartManagerTestFixture& fixture)
{
    BOOST_TEST_MESSAGE("2nd update cart...");

    fixture.resetInstance();

    fixture.updateCart("1", "1 3 5 7 9");
    fixture.updateCart("2", "");
    fixture.updateCart("7", "1 5 1 5");
    fixture.updateCart("10", "");
    fixture.updateCart("1000", "1001 1010 1100");
    fixture.updateCart("10000", "10003 10004 10005 10006");

    fixture.checkCartManager();
}

}

BOOST_AUTO_TEST_SUITE(CartManagerTest)

BOOST_FIXTURE_TEST_CASE(checkLocalCartManager, CartManagerTestFixture)
{
    BOOST_REQUIRE(initLocalStorage(COLLECTION_NAME, TEST_DIR_STR));

    testCart1(*this);
    testCart2(*this);
}

BOOST_FIXTURE_TEST_CASE(checkRemoteCartManager, CartManagerTestFixture)
{
    if (! initRemoteStorage(COLLECTION_NAME, TEST_DIR_STR,
                            KEYSPACE_NAME, REMOTE_STORAGE_URL))
    {
        cerr << "warning: exit test case as failed to connect " << REMOTE_STORAGE_URL << endl;
        return;
    }

    testCart1(*this);
    testCart2(*this);
}

BOOST_FIXTURE_TEST_CASE(checkCassandraNotConnect, CartManagerTestFixture)
{
    BOOST_REQUIRE(! initRemoteStorage(COLLECTION_NAME, TEST_DIR_STR,
                                      KEYSPACE_NAME, REMOTE_STORAGE_URL_NOT_CONNECT));

    resetInstance();

    string userId = "aaa";
    std::vector<itemid_t> cartItems;
    split_str_to_items("1 2 3", cartItems);

    BOOST_CHECK(cartManager_->updateCart(userId, cartItems) == false);

    std::vector<itemid_t> getItems;
    BOOST_CHECK(cartManager_->getCart(userId, getItems) == false);
}

BOOST_AUTO_TEST_SUITE_END() 
