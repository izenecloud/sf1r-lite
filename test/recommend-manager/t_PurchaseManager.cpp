/// @file t_PurchaseManager.cpp
/// @brief test PurchaseManager in purchase operations
/// @author Jun Jiang
/// @date Created 2011-04-19
///

#include "PurchaseManagerTestFixture.h"
#include "test_util.h"
#include <recommend-manager/storage/PurchaseManager.h>

#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>
#include <iostream>

using namespace std;
using namespace sf1r;

namespace
{
const string TEST_DIR_STR = "recommend_test/t_PurchaseManager";
const string KEYSPACE_NAME = "test_recommend";
const string COLLECTION_NAME = "example";

void testPurchase1(PurchaseManagerTestFixture& fixture)
{
    BOOST_TEST_MESSAGE("1st add purchase...");

    fixture.resetInstance();

    fixture.addPurchaseItem("1", "20 10 40");
    fixture.addPurchaseItem("1", "10");
    fixture.addPurchaseItem("2", "20 30");
    fixture.addPurchaseItem("3", "20 30 20");

    fixture.addRandItem("7", 99);
    fixture.addRandItem("6", 100);
    fixture.addRandItem("5", 101);
    fixture.addRandItem("4", 1234);

    fixture.checkPurchaseManager();
}

void testPurchase2(PurchaseManagerTestFixture& fixture)
{
    BOOST_TEST_MESSAGE("2nd add purchase...");

    fixture.resetInstance();

    fixture.addPurchaseItem("1", "40 50 60");
    fixture.addPurchaseItem("2", "40");
    fixture.addPurchaseItem("3", "30 40 50");
    fixture.addPurchaseItem("4", "20 30");

    fixture.checkPurchaseManager();
}

}

BOOST_AUTO_TEST_SUITE(PurchaseManagerTest)

BOOST_FIXTURE_TEST_CASE(checkLocalPurchaseManager, PurchaseManagerTestFixture)
{
    BOOST_REQUIRE(initLocalStorage(COLLECTION_NAME, TEST_DIR_STR));

    testPurchase1(*this);
    testPurchase2(*this);
}

BOOST_FIXTURE_TEST_CASE(checkRemotePurchaseManager, PurchaseManagerTestFixture)
{
    if (! initRemoteStorage(COLLECTION_NAME, TEST_DIR_STR,
                            KEYSPACE_NAME, REMOTE_STORAGE_URL))
    {
        cerr << "warning: exit test case as failed to connect " << REMOTE_STORAGE_URL << endl;
        return;
    }

    testPurchase1(*this);
    testPurchase2(*this);
}

BOOST_FIXTURE_TEST_CASE(checkCassandraNotConnect, PurchaseManagerTestFixture)
{
    BOOST_REQUIRE(! initRemoteStorage(COLLECTION_NAME, TEST_DIR_STR,
                                      KEYSPACE_NAME, REMOTE_STORAGE_URL_NOT_CONNECT));

    resetInstance();

    string userId = "aaa";
    std::vector<itemid_t> orderItemVec;
    split_str_to_items("1 2 3", orderItemVec);

    BOOST_CHECK(purchaseManager_->addPurchaseItem(userId, orderItemVec, NULL) == false);

    ItemIdSet itemIdSet;
    BOOST_CHECK(purchaseManager_->getPurchaseItemSet(userId, itemIdSet) == false);
}

BOOST_AUTO_TEST_SUITE_END() 
