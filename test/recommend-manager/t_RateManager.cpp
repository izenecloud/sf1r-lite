/// @file t_RateManager.cpp
/// @brief test RateManager in rating items
/// @author Jun Jiang
/// @date Created 2011-10-17
///

#include "RateManagerTestFixture.h"
#include <recommend-manager/storage/RateManager.h>

#include <boost/test/unit_test.hpp>
#include <string>

using namespace std;
using namespace sf1r;

namespace
{
const char* TEST_DIR_STR = "recommend_test/t_RateManager";
const string KEYSPACE_NAME = "test_recommend";
const string COLLECTION_NAME = "example";

void testRate1(RateManagerTestFixture& fixture)
{
    BOOST_TEST_MESSAGE("1st update rate...");

    fixture.resetInstance();

    fixture.addRate("1", 1, 1);
    fixture.addRate("1", 2, 2);
    fixture.addRate("1", 3, 3);
    fixture.removeRate("1", 3);
    fixture.removeRate("1", 5);
    fixture.addRate("1", 4, 4);
    fixture.addRate("1", 5, 5);

    fixture.addRate("2", 5, 3);
    fixture.addRate("2", 1, 5);

    fixture.addRandRate("7", 99);
    fixture.addRandRate("6", 100);
    fixture.addRandRate("5", 101);
    fixture.addRandRate("4", 1234);

    fixture.checkRateManager();
}

void testRate2(RateManagerTestFixture& fixture)
{
    BOOST_TEST_MESSAGE("2nd update rate...");

    fixture.resetInstance();

    fixture.removeRate("1", 2);
    fixture.removeRate("1", 1);
    fixture.addRate("1", 10, 5);
    fixture.addRate("1", 8, 4);
    fixture.addRate("1", 20, 3);
    fixture.addRate("1", 1000, 2);

    fixture.addRate("2", 1000, 3);
    fixture.addRate("2", 100, 1);
    fixture.removeRate("2", 1);
    fixture.removeRate("2", 10);
    fixture.removeRate("2", 1000);
    fixture.addRate("2", 100, 4);
    fixture.addRate("2", 1000, 4);

    fixture.removeRate("7", 1);
    fixture.removeRate("6", 10);
    fixture.removeRate("5", 100);
    fixture.removeRate("4", 1000);

    fixture.checkRateManager();
}

}

BOOST_AUTO_TEST_SUITE(RateManagerTest)

BOOST_FIXTURE_TEST_CASE(checkLocalRateManager, RateManagerTestFixture)
{
    BOOST_REQUIRE(initLocalStorage(COLLECTION_NAME, TEST_DIR_STR));

    testRate1(*this);
    testRate2(*this);
}

BOOST_FIXTURE_TEST_CASE(checkRemoteRateManager, RateManagerTestFixture)
{
    if (! initRemoteStorage(COLLECTION_NAME, TEST_DIR_STR,
                            KEYSPACE_NAME, REMOTE_STORAGE_URL))
    {
        cerr << "warning: exit test case as failed to connect " << REMOTE_STORAGE_URL << endl;
        return;
    }

    testRate1(*this);
    testRate2(*this);
}

BOOST_FIXTURE_TEST_CASE(checkCassandraNotConnect, RateManagerTestFixture)
{
    BOOST_REQUIRE(! initRemoteStorage(COLLECTION_NAME, TEST_DIR_STR,
                                      KEYSPACE_NAME, REMOTE_STORAGE_URL_NOT_CONNECT));

    resetInstance();

    string userId = "aaa";
    itemid_t itemId = 1;
    rate_t rate = 5;

    BOOST_CHECK(rateManager_->addRate(userId, itemId, rate) == false);
    BOOST_CHECK(rateManager_->removeRate(userId, itemId) == false);

    ItemRateMap itemRateMap;
    BOOST_CHECK(rateManager_->getItemRateMap(userId, itemRateMap) == false);
}

BOOST_AUTO_TEST_SUITE_END() 
