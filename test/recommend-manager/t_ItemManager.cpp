///
/// @file t_ItemManager.cpp
/// @brief test ItemManager in Item operations
/// @author Jun Jiang
/// @date Created 2011-04-19
///

#include "ItemManagerTestFixture.h"
#include "SearchMasterItemManagerTestFixture.h"
#include "test_util.h"
#include <recommend-manager/item/ItemManager.h>

#include <boost/test/unit_test.hpp>

using namespace sf1r;

namespace
{
const char* TEST_DIR_STR = "recommend_test/t_ItemManager";
}

BOOST_AUTO_TEST_SUITE(ItemManagerTest)

BOOST_FIXTURE_TEST_CASE(checkItem, ItemManagerTestFixture)
{
    setTestDir(TEST_DIR_STR);

    resetInstance();
    checkItemManager();

    createItems(10);
    checkItemManager();

    updateItems();
    checkItemManager();

    resetInstance();
    checkItemManager();

    createItems(1000);
    checkItemManager();

    removeItems();
    checkItemManager();

    updateItems();
    checkItemManager();
}

BOOST_FIXTURE_TEST_CASE(checkSearchMasterItemManager, SearchMasterItemManagerTestFixture)
{
    string baseDir = TEST_DIR_STR;
    setTestDir(baseDir);

    string logServerDir = baseDir + "/log_server";
    create_empty_directory(logServerDir);
    BOOST_REQUIRE(startRpcLogServer(logServerDir));

    resetInstance();
    checkItemManager();

    createItems(10);
    checkItemManager();

    updateItems();
    checkItemManager();

    createItems(1000);
    checkItemManager();

    removeItems();
    checkItemManager();

    updateItems();
    checkItemManager();
}

BOOST_AUTO_TEST_SUITE_END() 
