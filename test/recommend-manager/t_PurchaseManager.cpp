/// @file t_PurchaseManager.cpp
/// @brief test PurchaseManager in purchase operations
/// @author Jun Jiang
/// @date Created 2011-04-19
///

#include "PurchaseManagerTestFixture.h"
#include <recommend-manager/storage/LocalPurchaseManager.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const char* TEST_DIR_STR = "recommend_test/t_PurchaseManager";
const char* PURCHASE_DB_STR = "purchase.db";
}

BOOST_AUTO_TEST_SUITE(PurchaseManagerTest)

BOOST_FIXTURE_TEST_CASE(checkPurchase, PurchaseManagerTestFixture)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);
    bfs::path purchasePath(bfs::path(TEST_DIR_STR) / PURCHASE_DB_STR);

    {
        BOOST_TEST_MESSAGE("1st add purchase...");

        LocalPurchaseManager purchaseManager(purchasePath.string());
        setPurchaseManager(&purchaseManager);

        addPurchaseItem("1", "20 10 40");
        addPurchaseItem("1", "10");
        addPurchaseItem("2", "20 30");

        addPurchaseItem("1", "30");
        addPurchaseItem("3", "20 30 20");

        addPurchaseItem("1", "10 10");
        addPurchaseItem("2", "20");
        addPurchaseItem("3", "30");

        checkPurchaseManager();
    }

    {
        BOOST_TEST_MESSAGE("2nd add purchase...");

        LocalPurchaseManager purchaseManager(purchasePath.string());
        setPurchaseManager(&purchaseManager);

        checkPurchaseManager();

        addPurchaseItem("1", "40 50 60");
        addPurchaseItem("2", "40");
        addPurchaseItem("3", "30 40 50");
        addPurchaseItem("4", "20 30");

        checkPurchaseManager();
    }
}

BOOST_AUTO_TEST_SUITE_END() 
