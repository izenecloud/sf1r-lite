///
/// @file t_VisitManager.cpp
/// @brief test VisitManager in visit operations
/// @author Jun Jiang
/// @date Created 2011-04-19
///

#include <util/ustring/UString.h>
#include <recommend-manager/VisitManager.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm> // sort

using namespace std;
using namespace boost;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
const char* TEST_DIR_STR = "recommend_test/t_VisitManager";
const char* VISIT_DB_STR = "visit.db";
const char* SESSION_DB_STR = "session.db";
const char* COVISIT_DIR_STR = "covisit";
}

typedef map<userid_t, set<itemid_t> > VisitMap;

void checkVisitManager(const VisitMap& visitMap, VisitManager& visitManager)
{
    BOOST_CHECK_EQUAL(visitManager.visitUserNum(), visitMap.size());

    for (VisitMap::const_iterator it = visitMap.begin();
        it != visitMap.end(); ++it)
    {
        ItemIdSet itemIdSet;
        BOOST_CHECK(visitManager.getVisitItemSet(it->first, itemIdSet));
        BOOST_CHECK_EQUAL_COLLECTIONS(itemIdSet.begin(), itemIdSet.end(),
                                      it->second.begin(), it->second.end());
    }
}

void iterateVisitManager(const VisitMap& visitMap, VisitManager& visitManager)
{
    BOOST_CHECK_EQUAL(visitManager.visitUserNum(), visitMap.size());

    unsigned int iterNum = 0;
    for (VisitManager::SDBIterator visitIt = visitManager.begin();
        visitIt != visitManager.end(); ++visitIt)
    {
        userid_t userId = visitIt->first;
        const ItemIdSet& itemIdSet = visitIt->second;

        VisitMap::const_iterator it = visitMap.find(userId);
        BOOST_CHECK(it != visitMap.end());
        BOOST_CHECK_EQUAL_COLLECTIONS(itemIdSet.begin(), itemIdSet.end(),
                                      it->second.begin(), it->second.end());
        ++iterNum;
    }

    BOOST_TEST_MESSAGE("iterNum: " << iterNum);
    BOOST_CHECK_EQUAL(iterNum, visitManager.visitUserNum());
}

void checkCoVisitResult(
    CoVisitManager* coVisitManager,
    itemid_t inputItem,
    const char* recItemStr
)
{
    vector<itemid_t> goldRecItems;
    istringstream ss(recItemStr);
    itemid_t itemId;
    while (ss >> itemId)
    {
        goldRecItems.push_back(itemId);
    }
    sort(goldRecItems.begin(), goldRecItems.end());

    vector<itemid_t> results;
    coVisitManager->getCoVisitation(goldRecItems.size() + 10,
                                    inputItem, results);
    sort(results.begin(), results.end());

    BOOST_CHECK_EQUAL_COLLECTIONS(results.begin(), results.end(),
                                  goldRecItems.begin(), goldRecItems.end());
}

BOOST_AUTO_TEST_SUITE(VisitManagerTest)

BOOST_AUTO_TEST_CASE(checkVisit)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

    bfs::path visitDBPath(bfs::path(TEST_DIR_STR) / VISIT_DB_STR);
    bfs::path sessionDBPath(bfs::path(TEST_DIR_STR) / SESSION_DB_STR);

    typedef pair<userid_t, itemid_t> VisitPair;
    vector<VisitPair> visitVec;
    visitVec.push_back(VisitPair(1, 10));
    visitVec.push_back(VisitPair(2, 20));
    visitVec.push_back(VisitPair(1, 20));
    visitVec.push_back(VisitPair(2, 30));
    visitVec.push_back(VisitPair(3, 30));
    visitVec.push_back(VisitPair(1, 30));

    // visit duplicate item
    visitVec.push_back(VisitPair(1, 10));
    visitVec.push_back(VisitPair(2, 30));
    visitVec.push_back(VisitPair(3, 30));

    VisitMap visitMap;
    for (vector<VisitPair>::const_iterator it = visitVec.begin();
        it != visitVec.end(); ++it)
    {
        visitMap[it->first].insert(it->second);
    }

    bfs::path covisitPath(bfs::path(TEST_DIR_STR) / COVISIT_DIR_STR);
    CoVisitManager* coVisitManager = new CoVisitManager(covisitPath.string());

    {
        BOOST_TEST_MESSAGE("add visit...");
        VisitManager visitManager(visitDBPath.string(), sessionDBPath.string(), coVisitManager);
        string sessionId("session_001");
        for (vector<VisitPair>::const_iterator it = visitVec.begin();
                it != visitVec.end(); ++it)
        {
            BOOST_CHECK(visitManager.addVisitItem(sessionId, it->first, it->second));
        }

        checkVisitManager(visitMap, visitManager);
        iterateVisitManager(visitMap, visitManager);
        checkCoVisitResult(coVisitManager, 10, "20 30");
        checkCoVisitResult(coVisitManager, 20, "10 30");
        checkCoVisitResult(coVisitManager, 30, "10 20");
        checkCoVisitResult(coVisitManager, 40, "");

        visitManager.flush();
    }

    {
        BOOST_TEST_MESSAGE("continue add visit...");

        VisitManager visitManager(visitDBPath.string(), sessionDBPath.string(), coVisitManager);
        checkVisitManager(visitMap, visitManager);
        iterateVisitManager(visitMap, visitManager);

        vector<VisitPair> newVisitVec;
        newVisitVec.push_back(VisitPair(3, 10));
        newVisitVec.push_back(VisitPair(2, 10));
        newVisitVec.push_back(VisitPair(3, 20));
        newVisitVec.push_back(VisitPair(4, 20));
        newVisitVec.push_back(VisitPair(4, 40));

        string sessionId("session_002");
        for (vector<VisitPair>::const_iterator it = newVisitVec.begin();
                it != newVisitVec.end(); ++it)
        {
            visitMap[it->first].insert(it->second);
            BOOST_CHECK(visitManager.addVisitItem(sessionId, it->first, it->second));
        }

        checkVisitManager(visitMap, visitManager);
        iterateVisitManager(visitMap, visitManager);
        checkCoVisitResult(coVisitManager, 10, "20 30");
        checkCoVisitResult(coVisitManager, 20, "10 30 40");
        checkCoVisitResult(coVisitManager, 30, "10 20");
        checkCoVisitResult(coVisitManager, 40, "20");

        visitManager.flush();
    }

    delete coVisitManager;
}

BOOST_AUTO_TEST_SUITE_END() 
