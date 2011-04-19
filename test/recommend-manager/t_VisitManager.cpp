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

using namespace std;
using namespace boost;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
const char* TEST_DIR_STR = "recommend_test";
const char* VISIT_DB_STR = "visit.db";
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

    int iterNum = 0;
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

BOOST_AUTO_TEST_SUITE(VisitManagerTest)

BOOST_AUTO_TEST_CASE(checkVisit)
{
    bfs::path visitPath(bfs::path(TEST_DIR_STR) / VISIT_DB_STR);
    boost::filesystem::remove(visitPath);
    bfs::create_directories(TEST_DIR_STR);

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

    {
        BOOST_TEST_MESSAGE("add visit...");
        VisitManager visitManager(visitPath.string());
        for (vector<VisitPair>::const_iterator it = visitVec.begin();
                it != visitVec.end(); ++it)
        {
            BOOST_CHECK(visitManager.addVisitItem(it->first, it->second));
        }

        checkVisitManager(visitMap, visitManager);
        iterateVisitManager(visitMap, visitManager);

        visitManager.flush();
    }

    {
        BOOST_TEST_MESSAGE("continue add visit...");

        VisitManager visitManager(visitPath.string());
        checkVisitManager(visitMap, visitManager);
        iterateVisitManager(visitMap, visitManager);

        vector<VisitPair> newVisitVec;
        newVisitVec.push_back(VisitPair(3, 10));
        newVisitVec.push_back(VisitPair(2, 10));
        newVisitVec.push_back(VisitPair(3, 20));
        newVisitVec.push_back(VisitPair(4, 40));

        // visit duplicate item
        visitVec.push_back(VisitPair(1, 10));
        visitVec.push_back(VisitPair(2, 20));
        visitVec.push_back(VisitPair(3, 30));

        for (vector<VisitPair>::const_iterator it = newVisitVec.begin();
                it != newVisitVec.end(); ++it)
        {
            visitMap[it->first].insert(it->second);
            BOOST_CHECK(visitManager.addVisitItem(it->first, it->second));
        }

        checkVisitManager(visitMap, visitManager);
        iterateVisitManager(visitMap, visitManager);

        visitManager.flush();
    }
}

BOOST_AUTO_TEST_SUITE_END() 
