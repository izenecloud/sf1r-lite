///
/// @file t_VisitManager.cpp
/// @brief test VisitManager in visit operations
/// @author Jun Jiang
/// @date Created 2011-04-19
///

#include "VisitManagerTestFixture.h"
#include <recommend-manager/storage/RecommendStorageFactory.h>
#include <configuration-manager/CassandraStorageConfig.h>
#include <recommend-manager/storage/LocalVisitManager.h>
#include <recommend-manager/storage/RemoteVisitManager.h>
#include <recommend-manager/VisitMatrix.h>
#include <recommend-manager/storage/CassandraAdaptor.h>
#include <log-manager/CassandraConnection.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>

#include <string>
#include <vector>
#include <sstream>
#include <algorithm> // sort

using namespace std;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const string TEST_DIR_STR = "recommend_test/t_VisitManager";
const string TEST_CASSANDRA_URL = "cassandra://localhost";
const string KEYSPACE_NAME = "test_recommend";
const string COLLECTION_NAME = "example";
const string COVISIT_DIR_STR = "covisit";

void checkCoVisitManager(
    CoVisitManager& coVisitManager,
    itemid_t inputItem,
    const char* recItemStr
)
{
    std::vector<itemid_t> goldRecItems;
    std::istringstream iss(recItemStr);
    itemid_t itemId;
    while (iss >> itemId)
    {
        goldRecItems.push_back(itemId);
    }
    std::sort(goldRecItems.begin(), goldRecItems.end());

    std::vector<itemid_t> results;
    coVisitManager.getCoVisitation(goldRecItems.size() + 10,
                                   inputItem, results);
    std::sort(results.begin(), results.end());

    BOOST_CHECK_EQUAL_COLLECTIONS(results.begin(), results.end(),
                                  goldRecItems.begin(), goldRecItems.end());
}

template<class VisitManagerType>
void testVisit1(
    RecommendStorageFactory& factory,
    CoVisitManager& coVisitManager,
    VisitManagerTestFixture& fixture
)
{
    BOOST_TEST_MESSAGE("1st add visit...");

    boost::scoped_ptr<VisitManager> visitManager(factory.createVisitManager());
    BOOST_CHECK(dynamic_cast<VisitManagerType*>(visitManager.get()) != NULL);
    fixture.setVisitManager(visitManager.get());

    VisitMatrix visitMatrix(coVisitManager);
    std::string sessionId = "session_001";

    fixture.addVisitItem(sessionId, "1", 10, &visitMatrix);
    fixture.addVisitItem(sessionId, "2", 20, &visitMatrix);
    fixture.addVisitItem(sessionId, "1", 20, &visitMatrix);
    fixture.addVisitItem(sessionId, "2", 30, &visitMatrix);
    fixture.addVisitItem(sessionId, "3", 30, &visitMatrix);
    fixture.addVisitItem(sessionId, "1", 30, &visitMatrix);

    // visit duplicate item
    fixture.addVisitItem(sessionId, "1", 10, &visitMatrix);
    fixture.addVisitItem(sessionId, "2", 30, &visitMatrix);
    fixture.addVisitItem(sessionId, "3", 30, &visitMatrix);

    fixture.visitRecommendItem("1", 10);
    fixture.visitRecommendItem("1", 20);
    fixture.visitRecommendItem("3", 30);

    fixture.addRandItem(sessionId, "1", 99);
    fixture.addRandItem(sessionId, "2", 100);
    fixture.addRandItem(sessionId, "3", 101);
    fixture.addRandItem(sessionId, "4", 2468);

    fixture.checkVisitManager();

    checkCoVisitManager(coVisitManager, 10, "20 30");
    checkCoVisitManager(coVisitManager, 20, "10 30");
    checkCoVisitManager(coVisitManager, 30, "10 20");
    checkCoVisitManager(coVisitManager, 40, "");
}

template<class VisitManagerType>
void testVisit2(
    RecommendStorageFactory& factory,
    CoVisitManager& coVisitManager,
    VisitManagerTestFixture& fixture
)
{
    BOOST_TEST_MESSAGE("2nd add visit...");

    boost::scoped_ptr<VisitManager> visitManager(factory.createVisitManager());
    BOOST_CHECK(dynamic_cast<VisitManagerType*>(visitManager.get()) != NULL);
    fixture.setVisitManager(visitManager.get());

    VisitMatrix visitMatrix(coVisitManager);
    std::string sessionId = "session_002";

    fixture.addVisitItem(sessionId, "3", 10, &visitMatrix);
    fixture.addVisitItem(sessionId, "2", 10, &visitMatrix);
    fixture.addVisitItem(sessionId, "3", 20, &visitMatrix);
    fixture.addVisitItem(sessionId, "4", 20, &visitMatrix);
    fixture.addVisitItem(sessionId, "4", 40, &visitMatrix);

    fixture.visitRecommendItem("3", 10);
    fixture.visitRecommendItem("4", 20);

    fixture.checkVisitManager();

    checkCoVisitManager(coVisitManager, 10, "20 30");
    checkCoVisitManager(coVisitManager, 20, "10 30 40");
    checkCoVisitManager(coVisitManager, 30, "10 20");
    checkCoVisitManager(coVisitManager, 40, "20");
}

}

BOOST_AUTO_TEST_SUITE(VisitManagerTest)

BOOST_FIXTURE_TEST_CASE(checkLocalVisitManager, VisitManagerTestFixture)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

    CassandraStorageConfig config;
    RecommendStorageFactory factory(config, COLLECTION_NAME, TEST_DIR_STR);

    bfs::path covisitPath(bfs::path(TEST_DIR_STR) / COVISIT_DIR_STR);
    CoVisitManager coVisitManager(covisitPath.string());

    testVisit1<LocalVisitManager>(factory, coVisitManager, *this);
    testVisit2<LocalVisitManager>(factory, coVisitManager, *this);
}

BOOST_FIXTURE_TEST_CASE(checkRemoteVisitManager, VisitManagerTestFixture)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

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
        CassandraAdaptor(factory.getVisitItemColumnFamily(), client).dropColumnFamily();
        CassandraAdaptor(factory.getVisitRecommendColumnFamily(), client).dropColumnFamily();
        CassandraAdaptor(factory.getVisitSessionColumnFamily(), client).dropColumnFamily();
    }

    bfs::path covisitPath(bfs::path(TEST_DIR_STR) / COVISIT_DIR_STR);
    CoVisitManager coVisitManager(covisitPath.string());

    testVisit1<RemoteVisitManager>(factory, coVisitManager, *this);
    testVisit2<RemoteVisitManager>(factory, coVisitManager, *this);
}

BOOST_AUTO_TEST_CASE(checkCassandraNotConnect)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

    CassandraConnection& connection = CassandraConnection::instance();
    const char* TEST_CASSANDRA_NOT_CONNECT_URL = "cassandra://localhost:9161";
    BOOST_CHECK(connection.init(TEST_CASSANDRA_NOT_CONNECT_URL) == false);

    CassandraStorageConfig config;
    config.enable = true;
    config.keyspace = KEYSPACE_NAME;
    RecommendStorageFactory factory(config, COLLECTION_NAME, TEST_DIR_STR);

    bfs::path covisitPath(bfs::path(TEST_DIR_STR) / COVISIT_DIR_STR);
    CoVisitManager coVisitManager(covisitPath.string());
    VisitMatrix visitMatrix(coVisitManager);

    boost::scoped_ptr<VisitManager> visitManager(factory.createVisitManager());

    string sessionId = "session_001";
    string userId = "aaa";
    itemid_t itemId = 1;

    BOOST_CHECK(visitManager->addVisitItem(sessionId, userId, itemId, &visitMatrix) == false);
    BOOST_CHECK(visitManager->visitRecommendItem(userId, itemId) == false);

    ItemIdSet itemIdSet;
    BOOST_CHECK(visitManager->getVisitItemSet(userId, itemIdSet) == false);
    BOOST_CHECK(visitManager->getRecommendItemSet(userId, itemIdSet) == false);

    VisitSession visitSession;
    BOOST_CHECK(visitManager->getVisitSession(userId, visitSession) == false);
}

BOOST_AUTO_TEST_SUITE_END() 
