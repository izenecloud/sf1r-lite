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
const char* RECOMMEND_DB_STR = "recommend.db";
const char* SESSION_DB_STR = "session.db";
const char* COVISIT_DIR_STR = "covisit";
}

struct VisitInput
{
    string userId_;
    itemid_t itemId_;
    bool isRec_;

    VisitInput(const string& userId, itemid_t itemId, bool isRec)
        : userId_(userId)
        , itemId_(itemId)
        , isRec_(isRec)
    {}
};

typedef vector<VisitInput> VisitInputVec;
typedef map<string, set<itemid_t> > VisitMap;
typedef map<string, set<itemid_t> > SessionMap;
typedef map<string, set<itemid_t> > RecommendMap;

void addVisitInput(
    const string& sessionId,
    const VisitInputVec& visitInputVec,
    VisitManager& visitManager,
    VisitMap& visitMap,
    SessionMap& sessionMap,
    RecommendMap& recommendMap
)
{
    set<string> userSet;
    for (VisitInputVec::const_iterator it = visitInputVec.begin();
        it != visitInputVec.end(); ++it)
    {
        visitMap[it->userId_].insert(it->itemId_);

        // clear items in previous user session
        if (userSet.insert(it->userId_).second)
        {
            sessionMap[it->userId_].clear();
        }
        sessionMap[it->userId_].insert(it->itemId_);

        if (it->isRec_)
        {
            recommendMap[it->userId_].insert(it->itemId_);
        }

        BOOST_CHECK(visitManager.addVisitItem(sessionId, it->userId_, it->itemId_, it->isRec_));
    }
}

void checkVisitSet(const VisitMap& visitMap, VisitManager& visitManager)
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

void checkSessionSet(const SessionMap& sessionMap, VisitManager& visitManager)
{
    for (SessionMap::const_iterator it = sessionMap.begin();
        it != sessionMap.end(); ++it)
    {
        VisitSession visitSession;
        BOOST_CHECK(visitManager.getVisitSession(it->first, visitSession));
        BOOST_CHECK_EQUAL_COLLECTIONS(visitSession.itemSet_.begin(), visitSession.itemSet_.end(),
                                      it->second.begin(), it->second.end());
    }
}

void checkRecommendSet(const RecommendMap& recommendMap, VisitManager& visitManager)
{
    for (RecommendMap::const_iterator it = recommendMap.begin();
        it != recommendMap.end(); ++it)
    {
        ItemIdSet itemIdSet;
        BOOST_CHECK(visitManager.getRecommendItemSet(it->first, itemIdSet));
        BOOST_CHECK_EQUAL_COLLECTIONS(itemIdSet.begin(), itemIdSet.end(),
                                      it->second.begin(), it->second.end());
    }
}

void iterateVisitManager(const VisitMap& visitMap, VisitManager& visitManager)
{
    BOOST_CHECK_EQUAL(visitManager.visitUserNum(), visitMap.size());

    unsigned int iterNum = 0;
    for (VisitManager::VisitIterator visitIt = visitManager.begin();
        visitIt != visitManager.end(); ++visitIt)
    {
        const string& userId = visitIt->first;
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
    CoVisitManager& coVisitManager,
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
    coVisitManager.getCoVisitation(goldRecItems.size() + 10,
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
    bfs::path recommendDBPath(bfs::path(TEST_DIR_STR) / RECOMMEND_DB_STR);
    bfs::path sessionDBPath(bfs::path(TEST_DIR_STR) / SESSION_DB_STR);
    bfs::path covisitPath(bfs::path(TEST_DIR_STR) / COVISIT_DIR_STR);

    CoVisitManager coVisitManager(covisitPath.string());
    VisitMap visitMap;
    SessionMap sessionMap;
    RecommendMap recommendMap;

    {
        BOOST_TEST_MESSAGE("add visit...");
        VisitManager visitManager(visitDBPath.string(), recommendDBPath.string(), sessionDBPath.string(), coVisitManager);

        // construct input
        VisitInputVec visitInputVec;
        visitInputVec.push_back(VisitInput("1", 10, false));
        visitInputVec.push_back(VisitInput("2", 20, false));
        visitInputVec.push_back(VisitInput("1", 20, true));
        visitInputVec.push_back(VisitInput("2", 30, false));
        visitInputVec.push_back(VisitInput("3", 30, true));
        visitInputVec.push_back(VisitInput("1", 30, false));
        // visit duplicate item
        visitInputVec.push_back(VisitInput("1", 10, true));
        visitInputVec.push_back(VisitInput("2", 30, false));
        visitInputVec.push_back(VisitInput("3", 30, true));

        addVisitInput("session_001", visitInputVec, visitManager,
                      visitMap, sessionMap, recommendMap);

        checkVisitSet(visitMap, visitManager);
        checkSessionSet(sessionMap, visitManager);
        checkRecommendSet(recommendMap, visitManager);
        iterateVisitManager(visitMap, visitManager);
        checkCoVisitResult(coVisitManager, 10, "20 30");
        checkCoVisitResult(coVisitManager, 20, "10 30");
        checkCoVisitResult(coVisitManager, 30, "10 20");
        checkCoVisitResult(coVisitManager, 40, "");

        visitManager.flush();
    }

    {
        BOOST_TEST_MESSAGE("continue add visit...");

        VisitManager visitManager(visitDBPath.string(), recommendDBPath.string(), sessionDBPath.string(), coVisitManager);
        checkVisitSet(visitMap, visitManager);
        checkSessionSet(sessionMap, visitManager);
        checkRecommendSet(recommendMap, visitManager);
        iterateVisitManager(visitMap, visitManager);

        // construct input
        VisitInputVec visitInputVec;
        visitInputVec.push_back(VisitInput("3", 10, true));
        visitInputVec.push_back(VisitInput("2", 10, false));
        visitInputVec.push_back(VisitInput("3", 20, false));
        visitInputVec.push_back(VisitInput("4", 20, true));
        visitInputVec.push_back(VisitInput("4", 40, false));

        addVisitInput("session_002", visitInputVec, visitManager,
                      visitMap, sessionMap, recommendMap);

        checkVisitSet(visitMap, visitManager);
        checkSessionSet(sessionMap, visitManager);
        checkRecommendSet(recommendMap, visitManager);
        iterateVisitManager(visitMap, visitManager);
        checkCoVisitResult(coVisitManager, 10, "20 30");
        checkCoVisitResult(coVisitManager, 20, "10 30 40");
        checkCoVisitResult(coVisitManager, 30, "10 20");
        checkCoVisitResult(coVisitManager, 40, "20");

        visitManager.flush();
    }
}

BOOST_AUTO_TEST_SUITE_END() 
