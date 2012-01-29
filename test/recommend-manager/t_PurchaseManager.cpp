/// @file t_PurchaseManager.cpp
/// @brief test PurchaseManager in purchase operations
/// @author Jun Jiang
/// @date Created 2011-04-19
///

#include "PurchaseManagerTestFixture.h"
#include <recommend-manager/storage/RecommendStorageFactory.h>
#include <configuration-manager/CassandraStorageConfig.h>
#include <recommend-manager/storage/LocalPurchaseManager.h>
#include <recommend-manager/storage/RemotePurchaseManager.h>
#include <recommend-manager/storage/CassandraAdaptor.h>
#include <log-manager/CassandraConnection.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <string>

using namespace std;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const string TEST_DIR_STR = "recommend_test/t_PurchaseManager";
const string TEST_CASSANDRA_URL = "cassandra://localhost";
const string KEYSPACE_NAME = "test_recommend";
const string COLLECTION_NAME = "example";

template<class PurchaseManagerType>
void testPurchase1(
    RecommendStorageFactory& factory,
    PurchaseManagerTestFixture& fixture
)
{
    BOOST_TEST_MESSAGE("1st add purchase...");

    boost::scoped_ptr<PurchaseManager> purchaseManager(factory.createPurchaseManager());
    BOOST_CHECK(dynamic_cast<PurchaseManagerType*>(purchaseManager.get()) != NULL);
    fixture.setPurchaseManager(purchaseManager.get());

    fixture.addPurchaseItem("1", "20 10 40");
    fixture.addPurchaseItem("1", "10");
    fixture.addPurchaseItem("2", "20 30");
    fixture.addPurchaseItem("3", "20 30 20");

    fixture.addRandItem("1", 99);
    fixture.addRandItem("2", 100);
    fixture.addRandItem("3", 101);
    fixture.addRandItem("4", 1234);

    fixture.checkPurchaseManager();
}

template<class PurchaseManagerType>
void testPurchase2(
    RecommendStorageFactory& factory,
    PurchaseManagerTestFixture& fixture
)
{
    BOOST_TEST_MESSAGE("2nd add purchase...");

    boost::scoped_ptr<PurchaseManager> purchaseManager(factory.createPurchaseManager());
    BOOST_CHECK(dynamic_cast<PurchaseManagerType*>(purchaseManager.get()) != NULL);
    fixture.setPurchaseManager(purchaseManager.get());

    fixture.checkPurchaseManager();

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
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

    CassandraStorageConfig config;
    RecommendStorageFactory factory(config, COLLECTION_NAME, TEST_DIR_STR);

    testPurchase1<LocalPurchaseManager>(factory, *this);
    testPurchase2<LocalPurchaseManager>(factory, *this);
}

BOOST_FIXTURE_TEST_CASE(checkRemotePurchaseManager, PurchaseManagerTestFixture)
{
    CassandraConnection& connection = CassandraConnection::instance();
    if (! connection.init(TEST_CASSANDRA_URL))
    {
        cerr << "warning: exit test case as failed to connect " << TEST_CASSANDRA_URL << endl;
        return;
    }

    CassandraStorageConfig config;
    config.enable = true;
    config.keyspace = KEYSPACE_NAME;
    RecommendStorageFactory factory(config, COLLECTION_NAME, TEST_DIR_STR);

    {
        libcassandra::Cassandra* client = connection.getCassandraClient(KEYSPACE_NAME);
        CassandraAdaptor adaptor(factory.getPurchaseColumnFamily(), client);
        adaptor.dropColumnFamily();
    }

    testPurchase1<RemotePurchaseManager>(factory, *this);
    testPurchase2<RemotePurchaseManager>(factory, *this);
}

BOOST_AUTO_TEST_CASE(checkCassandraNotConnect)
{
    CassandraConnection& connection = CassandraConnection::instance();
    const char* TEST_CASSANDRA_NOT_CONNECT_URL = "cassandra://localhost:9161";
    BOOST_CHECK(connection.init(TEST_CASSANDRA_NOT_CONNECT_URL) == false);

    CassandraStorageConfig config;
    config.enable = true;
    config.keyspace = KEYSPACE_NAME;
    RecommendStorageFactory factory(config, COLLECTION_NAME, TEST_DIR_STR);

    boost::scoped_ptr<PurchaseManager> purchaseManager(factory.createPurchaseManager());

    string userId = "aaa";
    std::vector<itemid_t> orderItemVec;
    orderItemVec.push_back(1);
    orderItemVec.push_back(2);

    BOOST_CHECK(purchaseManager->addPurchaseItem(userId, orderItemVec, NULL) == false);

    ItemIdSet itemIdSet;
    BOOST_CHECK(purchaseManager->getPurchaseItemSet(userId, itemIdSet) == false);
}

BOOST_AUTO_TEST_SUITE_END() 
