///
/// @file t_CustomRankManager.cpp
/// @brief test set/get the docs custom ranking.
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-07-17
///

#include <mining-manager/custom-rank-manager/CustomRankManager.h>
#include <mining-manager/custom-rank-manager/CustomRankScorer.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>
#include "../recommend-manager/test_util.h"

namespace bfs = boost::filesystem;
using namespace sf1r;

namespace
{
const std::string TEST_DIR = "custom_rank_test";
const std::string DB_NAME = "db.bin";

void checkEmpty(
    CustomRankManager& customRankManager,
    const std::string& query
)
{
    BOOST_TEST_MESSAGE("check empty, query: " << query);

    CustomRankValue customValue;
    BOOST_CHECK(customRankManager.getCustomValue(query, customValue));

    BOOST_CHECK(customValue.topIds.empty());
    BOOST_CHECK(customValue.excludeIds.empty());

    boost::scoped_ptr<CustomRankScorer> scorer;
    scorer.reset(customRankManager.getScorer(query));
    BOOST_CHECK(scorer.get() == NULL);
}

void checkSet(
    CustomRankManager& customRankManager,
    const std::string& query,
    const std::string& topIdListStr,
    const std::string& excludeIdListStr
)
{
    BOOST_TEST_MESSAGE("check set, query: " << query);

    CustomRankValue customValue;
    split_str_to_items(topIdListStr, customValue.topIds);
    split_str_to_items(excludeIdListStr, customValue.excludeIds);

    BOOST_CHECK(customRankManager.setCustomValue(query, customValue));
}

void checkGet(
    CustomRankManager& customRankManager,
    const std::string& query,
    const std::string& topIdListStr,
    const std::string& excludeIdListStr,
    const std::string& zeroScoreDocIdListStr
)
{
    BOOST_TEST_MESSAGE("check get, query: " << query);

    CustomRankValue goldCustomValue;
    split_str_to_items(topIdListStr, goldCustomValue.topIds);
    split_str_to_items(excludeIdListStr, goldCustomValue.excludeIds);

    CustomRankValue customValue;
    BOOST_CHECK(customRankManager.getCustomValue(query, customValue));

    BOOST_CHECK_EQUAL_COLLECTIONS(customValue.topIds.begin(), customValue.topIds.end(),
                                  goldCustomValue.topIds.begin(), goldCustomValue.topIds.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(customValue.excludeIds.begin(), customValue.excludeIds.end(),
                                  goldCustomValue.excludeIds.begin(), goldCustomValue.excludeIds.end());

    boost::scoped_ptr<CustomRankScorer> scorer;
    scorer.reset(customRankManager.getScorer(query));
    BOOST_REQUIRE(scorer.get() != NULL);

    std::size_t docNum = goldCustomValue.topIds.size();
    for (CustomRankValue::DocIdList::const_iterator it = goldCustomValue.topIds.begin();
        it != goldCustomValue.topIds.end(); ++it)
    {
        score_t score = scorer->getScore(*it);
        score_t goldScore = CustomRankScorer::CUSTOM_RANK_BASE_SCORE + docNum;
        BOOST_CHECK_EQUAL(score, goldScore);
        --docNum;
    }

    CustomRankValue::DocIdList zeroScoreDocIdList;
    split_str_to_items(zeroScoreDocIdListStr, zeroScoreDocIdList);

    for (CustomRankValue::DocIdList::const_iterator it = zeroScoreDocIdList.begin();
        it != zeroScoreDocIdList.end(); ++it)
    {
        score_t score = scorer->getScore(*it);
        BOOST_CHECK_EQUAL(score, 0);
    }
}

} // namespace

BOOST_AUTO_TEST_SUITE(CustomRankManagerTest)

BOOST_AUTO_TEST_CASE(testNormalCase)
{
    bfs::path dir(TEST_DIR);
    bfs::remove_all(dir);
    bfs::create_directory(dir);

    bfs::path dbPath = dir / DB_NAME;
    std::string dbPathStr = dbPath.string();

    std::string query1("aaa");
    std::string query2("bbb");

    {
        CustomRankManager customRankManager(dbPathStr);

        checkEmpty(customRankManager, query1);

        checkSet(customRankManager, query1, "1 2 3", "101 102");
        checkGet(customRankManager, query1, "1 2 3", "101 102", "4 5 6");

        checkEmpty(customRankManager, query2);
    }

    {
        BOOST_TEST_MESSAGE("reset instance");
        CustomRankManager customRankManager(dbPathStr);

        checkGet(customRankManager, query1, "1 2 3", "101 102", "4 5 6");
        checkEmpty(customRankManager, query2);

        checkSet(customRankManager, query1, "13 12 15", "103");
        checkGet(customRankManager, query1, "13 12 15", "103", "1 2 3");

        checkSet(customRankManager, query2, "9 7 5 3 1 2 4 6 8", "");
        checkGet(customRankManager, query2, "9 7 5 3 1 2 4 6 8", "", "10 11 12");

        std::vector<std::string> queries;
        BOOST_CHECK(customRankManager.getQueries(queries));
        BOOST_REQUIRE_EQUAL(queries.size(), 2U);
        BOOST_CHECK_EQUAL(queries[0], query1);
        BOOST_CHECK_EQUAL(queries[1], query2);

        checkSet(customRankManager, query1, "", "");
        checkEmpty(customRankManager, query1);

        checkSet(customRankManager, query2, "", "");
        checkEmpty(customRankManager, query2);

        queries.clear();
        BOOST_CHECK(customRankManager.getQueries(queries));
        BOOST_CHECK_EQUAL(queries.size(), 0U);
    }
}

BOOST_AUTO_TEST_SUITE_END() 
