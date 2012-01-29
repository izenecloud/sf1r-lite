///
/// @file t_VisitManager.cpp
/// @brief test VisitManager in visit operations
/// @author Jun Jiang
/// @date Created 2011-04-19
///

#include "VisitManagerTestFixture.h"
#include <recommend-manager/VisitManager.h>
#include <recommend-manager/VisitMatrix.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <string>
#include <vector>
#include <sstream>
#include <algorithm> // sort

using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const char* TEST_DIR_STR = "recommend_test/t_VisitManager";
const char* VISIT_DB_STR = "visit.db";
const char* RECOMMEND_DB_STR = "recommend.db";
const char* SESSION_DB_STR = "session.db";
const char* COVISIT_DIR_STR = "covisit";

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

void testVisit1(
    VisitManager& visitManager,
    CoVisitManager& coVisitManager,
    VisitManagerTestFixture& fixture
)
{
    BOOST_TEST_MESSAGE("1st add visit...");

    fixture.setVisitManager(&visitManager);

    VisitMatrix visitMatrix(coVisitManager);
    std::string sessionId = "session_001";

    fixture.addVisitItem(sessionId, "1", 10, false, &visitMatrix);
    fixture.addVisitItem(sessionId, "2", 20, false, &visitMatrix);
    fixture.addVisitItem(sessionId, "1", 20, true, &visitMatrix);
    fixture.addVisitItem(sessionId, "2", 30, false, &visitMatrix);
    fixture.addVisitItem(sessionId, "3", 30, true, &visitMatrix);
    fixture.addVisitItem(sessionId, "1", 30, false, &visitMatrix);

    // visit duplicate item
    fixture.addVisitItem(sessionId, "1", 10, true, &visitMatrix);
    fixture.addVisitItem(sessionId, "2", 30, false, &visitMatrix);
    fixture.addVisitItem(sessionId, "3", 30, true, &visitMatrix);

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

void testVisit2(
    VisitManager& visitManager,
    CoVisitManager& coVisitManager,
    VisitManagerTestFixture& fixture
)
{
    BOOST_TEST_MESSAGE("2nd add visit...");

    fixture.setVisitManager(&visitManager);
    fixture.checkVisitManager();

    VisitMatrix visitMatrix(coVisitManager);
    std::string sessionId = "session_002";

    fixture.addVisitItem(sessionId, "3", 10, true, &visitMatrix);
    fixture.addVisitItem(sessionId, "2", 10, false, &visitMatrix);
    fixture.addVisitItem(sessionId, "3", 20, false, &visitMatrix);
    fixture.addVisitItem(sessionId, "4", 20, true, &visitMatrix);
    fixture.addVisitItem(sessionId, "4", 40, false, &visitMatrix);

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

    bfs::path visitDBPath(bfs::path(TEST_DIR_STR) / VISIT_DB_STR);
    bfs::path recommendDBPath(bfs::path(TEST_DIR_STR) / RECOMMEND_DB_STR);
    bfs::path sessionDBPath(bfs::path(TEST_DIR_STR) / SESSION_DB_STR);
    bfs::path covisitPath(bfs::path(TEST_DIR_STR) / COVISIT_DIR_STR);

    CoVisitManager coVisitManager(covisitPath.string());

    {
        VisitManager visitManager(visitDBPath.string(), recommendDBPath.string(), sessionDBPath.string());
        testVisit1(visitManager, coVisitManager, *this);
    }

    {
        VisitManager visitManager(visitDBPath.string(), recommendDBPath.string(), sessionDBPath.string());
        testVisit2(visitManager, coVisitManager, *this);
    }
}

BOOST_AUTO_TEST_SUITE_END() 
