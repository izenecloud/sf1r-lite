///
/// @file t_VisitManager.cpp
/// @brief test VisitManager in visit operations
/// @author Jun Jiang
/// @date Created 2011-04-19
///

#include "VisitManagerTestFixture.h"
#include "test_util.h"
#include <recommend-manager/storage/VisitManager.h>
#include <recommend-manager/storage/RecommendMatrix.h>

#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>
#include <iostream>
#include <algorithm> // sort

using namespace std;
using namespace sf1r;

namespace
{
const string TEST_DIR_STR = "recommend_test/t_VisitManager";
const string KEYSPACE_NAME = "test_recommend";
const string COLLECTION_NAME = "example";
const string COVISIT_DIR_STR = TEST_DIR_STR + "/covisit";

class VisitMatrixStub : public RecommendMatrix
{
public:
    VisitMatrixStub(CoVisitManager& coVisitManager)
        : coVisitManager_(coVisitManager)
    {}

    virtual void update(
        const std::list<itemid_t>& oldItems,
        const std::list<itemid_t>& newItems
    )
    {
        coVisitManager_.visit(oldItems, newItems);
    }

private:
    CoVisitManager& coVisitManager_;
};

void checkCoVisitManager(
    CoVisitManager& coVisitManager,
    itemid_t inputItem,
    const char* recItemStr
)
{
    std::vector<itemid_t> goldRecItems;
    split_str_to_items(recItemStr, goldRecItems);
    std::sort(goldRecItems.begin(), goldRecItems.end());

    idmlib::recommender::RecommendItemVec recItems;
    coVisitManager.getCoVisitation(goldRecItems.size() + 10,
                                   inputItem, recItems);

    std::vector<itemid_t> itemIds;
    for (idmlib::recommender::RecommendItemVec::const_iterator it = recItems.begin();
        it != recItems.end(); ++it)
    {
        itemIds.push_back(it->itemId_);
    }
    std::sort(itemIds.begin(), itemIds.end());

    BOOST_CHECK_EQUAL_COLLECTIONS(itemIds.begin(), itemIds.end(),
                                  goldRecItems.begin(), goldRecItems.end());
}

void testVisit1(
    CoVisitManager& coVisitManager,
    VisitManagerTestFixture& fixture
)
{
    BOOST_TEST_MESSAGE("1st add visit...");

    fixture.resetInstance();

    VisitMatrixStub visitMatrix(coVisitManager);
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

    fixture.addRandItem(sessionId, "7", 99);
    fixture.addRandItem(sessionId, "6", 100);
    fixture.addRandItem(sessionId, "5", 101);
    fixture.addRandItem(sessionId, "4", 1234);

    fixture.checkVisitManager();

    checkCoVisitManager(coVisitManager, 10, "20 30");
    checkCoVisitManager(coVisitManager, 20, "10 30");
    checkCoVisitManager(coVisitManager, 30, "10 20");
    checkCoVisitManager(coVisitManager, 40, "");
}

void testVisit2(
    CoVisitManager& coVisitManager,
    VisitManagerTestFixture& fixture
)
{
    BOOST_TEST_MESSAGE("2nd add visit...");

    fixture.resetInstance();

    VisitMatrixStub visitMatrix(coVisitManager);
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
    BOOST_REQUIRE(initLocalStorage(COLLECTION_NAME, TEST_DIR_STR));

    CoVisitManager coVisitManager(COVISIT_DIR_STR);

    testVisit1(coVisitManager, *this);
    testVisit2(coVisitManager, *this);
}

BOOST_FIXTURE_TEST_CASE(checkRemoteVisitManager, VisitManagerTestFixture)
{
    if (! initRemoteStorage(COLLECTION_NAME, TEST_DIR_STR,
                            KEYSPACE_NAME, REMOTE_STORAGE_URL))
    {
        cerr << "warning: exit test case as failed to connect " << REMOTE_STORAGE_URL << endl;
        return;
    }

    CoVisitManager coVisitManager(COVISIT_DIR_STR);

    testVisit1(coVisitManager, *this);
    testVisit2(coVisitManager, *this);
}

BOOST_FIXTURE_TEST_CASE(checkCassandraNotConnect, VisitManagerTestFixture)
{
    BOOST_REQUIRE(! initRemoteStorage(COLLECTION_NAME, TEST_DIR_STR,
                                      KEYSPACE_NAME, REMOTE_STORAGE_URL_NOT_CONNECT));

    resetInstance();

    CoVisitManager coVisitManager(COVISIT_DIR_STR);
    VisitMatrixStub visitMatrix(coVisitManager);

    string sessionId = "session_001";
    string userId = "aaa";
    itemid_t itemId = 1;

    BOOST_CHECK(visitManager_->addVisitItem(sessionId, userId, itemId, &visitMatrix) == false);
    BOOST_CHECK(visitManager_->visitRecommendItem(userId, itemId) == false);

    ItemIdSet itemIdSet;
    BOOST_CHECK(visitManager_->getVisitItemSet(userId, itemIdSet) == false);
    BOOST_CHECK(visitManager_->getRecommendItemSet(userId, itemIdSet) == false);

    VisitSession visitSession;
    BOOST_CHECK(visitManager_->getVisitSession(userId, visitSession) == false);
}

BOOST_AUTO_TEST_SUITE_END() 
