///
/// @file t_ItemManager.cpp
/// @brief test ItemManager in Item operations
/// @author Jun Jiang
/// @date Created 2011-04-19
///

#include "ItemManagerTestFixture.h"
#include "SearchMasterItemManagerTestFixture.h"
#include "RemoteItemManagerTestFixture.h"
#include "SearchMasterAddressFinderStub.h"
#include "test_util.h"
#include <aggregator-manager/MasterServerConnector.h>

#include <boost/test/unit_test.hpp>

using namespace sf1r;

namespace
{
const char* TEST_DIR_STR = "recommend_test/t_ItemManager";

void testItemManager(ItemManagerTestFixture& fixture)
{
    fixture.resetInstance();
    fixture.checkItemManager();

    fixture.createItems(10);
    fixture.checkItemManager();

    fixture.updateItems();
    fixture.checkItemManager();

    fixture.resetInstance();
    fixture.checkItemManager();

    fixture.createItems(1000);
    fixture.checkItemManager();

    fixture.removeItems();
    fixture.checkItemManager();

    fixture.updateItems();
    fixture.checkItemManager();
}

}

BOOST_AUTO_TEST_SUITE(ItemManagerTest)

BOOST_FIXTURE_TEST_CASE(checkLocalItemManager, ItemManagerTestFixture)
{
    setTestDir(TEST_DIR_STR);

    testItemManager(*this);
}

BOOST_FIXTURE_TEST_CASE(checkSearchMasterItemManager, SearchMasterItemManagerTestFixture)
{
    string baseDir = TEST_DIR_STR;
    setTestDir(baseDir);

    string logServerDir = baseDir + "/log_server";
    create_empty_directory(logServerDir);
    BOOST_REQUIRE(startRpcLogServer(logServerDir));

    testItemManager(*this);
}

BOOST_FIXTURE_TEST_CASE(checkRemoteItemManager, RemoteItemManagerTestFixture)
{
    string baseDir = TEST_DIR_STR;
    setTestDir(baseDir);

    string logServerDir = baseDir + "/log_server";
    create_empty_directory(logServerDir);
    BOOST_REQUIRE(startRpcLogServer(logServerDir));

    startSearchMasterServer();

    testItemManager(*this);
}

BOOST_FIXTURE_TEST_CASE(checkRemoteNotConnect, RemoteItemManagerTestFixture)
{
    string baseDir = TEST_DIR_STR;
    setTestDir(baseDir);

    string logServerDir = baseDir + "/log_server";
    create_empty_directory(logServerDir);
    BOOST_REQUIRE(startRpcLogServer(logServerDir));

    startSearchMasterServer();

    SearchMasterAddressFinderStub finderStub(addressFinderStub_);
    ++finderStub.port_;
    MasterServerConnector::get()->init(&finderStub);

    resetInstance();
    createItems(10);

    checkGetItemFail();
}

BOOST_FIXTURE_TEST_CASE(checkRemoteHostNotResolve, RemoteItemManagerTestFixture)
{
    string baseDir = TEST_DIR_STR;
    setTestDir(baseDir);

    string logServerDir = baseDir + "/log_server";
    create_empty_directory(logServerDir);
    BOOST_REQUIRE(startRpcLogServer(logServerDir));

    startSearchMasterServer();

    SearchMasterAddressFinderStub finderStub(addressFinderStub_);
    finderStub.host_.clear();
    MasterServerConnector::get()->init(&finderStub);

    resetInstance();
    createItems(10);

    checkGetItemFail();
}

BOOST_AUTO_TEST_SUITE_END() 
