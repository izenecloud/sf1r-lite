/// @file t_RateManager.cpp
/// @brief test RateManager in rating items
/// @author Jun Jiang
/// @date Created 2011-10-17
///

#include <recommend-manager/RateManager.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include <string>
#include <vector>
#include <map>

using namespace std;
using namespace boost;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const char* TEST_DIR_STR = "recommend_test/t_RateManager";
const char* RATE_DB_STR = "rate.db";
}

typedef map<string, ItemRateMap> UserRateMap;

void addRate(
    RateManager& rateManager,
    UserRateMap& userRateMap,
    const string& userId,
    itemid_t itemId,
    rate_t rate
)
{
    BOOST_CHECK(rateManager.addRate(userId, itemId, rate));
    userRateMap[userId][itemId] = rate;
}

void removeRate(
    RateManager& rateManager,
    UserRateMap& userRateMap,
    const string& userId,
    itemid_t itemId
)
{
    bool result = userRateMap[userId].erase(itemId);
    BOOST_CHECK_EQUAL(rateManager.removeRate(userId, itemId), result);
}

void checkRateManager(const UserRateMap& userRateMap, RateManager& rateManager)
{
    for (UserRateMap::const_iterator it = userRateMap.begin();
        it != userRateMap.end(); ++it)
    {
        ItemRateMap itemRateMap;
        BOOST_CHECK(rateManager.getItemRateMap(it->first, itemRateMap));

        const ItemRateMap& goldItemRateMap = it->second;
        BOOST_CHECK_EQUAL(itemRateMap.size(), goldItemRateMap.size());

        for (ItemRateMap::const_iterator rateIt = goldItemRateMap.begin();
            rateIt != goldItemRateMap.end(); ++rateIt)
        {
            BOOST_CHECK_EQUAL(itemRateMap[rateIt->first], rateIt->second);
        }
    }
}

BOOST_AUTO_TEST_SUITE(RateManagerTest)

BOOST_AUTO_TEST_CASE(checkRate)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

    bfs::path ratePath(bfs::path(TEST_DIR_STR) / RATE_DB_STR);
    bfs::create_directories(TEST_DIR_STR);

    UserRateMap userRateMap;

    {
        BOOST_TEST_MESSAGE("check empty rate...");
        RateManager rateManager(ratePath.string());

        BOOST_CHECK(userRateMap["1"].empty());
        BOOST_CHECK(userRateMap["10"].empty());
        BOOST_CHECK(userRateMap["100"].empty());

        checkRateManager(userRateMap, rateManager);
    }

    {
        BOOST_TEST_MESSAGE("update rate...");
        RateManager rateManager(ratePath.string());

        addRate(rateManager, userRateMap, "1", 1, 1);
        addRate(rateManager, userRateMap, "1", 2, 2);
        addRate(rateManager, userRateMap, "1", 3, 3);
        removeRate(rateManager, userRateMap, "1", 3);
        removeRate(rateManager, userRateMap, "1", 5);
        addRate(rateManager, userRateMap, "1", 4, 4);
        addRate(rateManager, userRateMap, "1", 5, 5);

        addRate(rateManager, userRateMap, "2", 5, 3);
        addRate(rateManager, userRateMap, "2", 1, 5);

        checkRateManager(userRateMap, rateManager);
    }

    {
        BOOST_TEST_MESSAGE("continue update rate...");
        RateManager rateManager(ratePath.string());

        checkRateManager(userRateMap, rateManager);

        removeRate(rateManager, userRateMap, "1", 2);
        removeRate(rateManager, userRateMap, "1", 1);
        addRate(rateManager, userRateMap, "1", 10, 5);
        addRate(rateManager, userRateMap, "1", 8, 4);
        addRate(rateManager, userRateMap, "1", 20, 3);
        addRate(rateManager, userRateMap, "1", 1000, 2);

        addRate(rateManager, userRateMap, "2", 1000, 3);
        addRate(rateManager, userRateMap, "2", 100, 1);
        removeRate(rateManager, userRateMap, "2", 1);
        removeRate(rateManager, userRateMap, "2", 10);
        removeRate(rateManager, userRateMap, "2", 1000);
        addRate(rateManager, userRateMap, "2", 100, 4);
        addRate(rateManager, userRateMap, "2", 1000, 4);

        checkRateManager(userRateMap, rateManager);
    }
}

BOOST_AUTO_TEST_SUITE_END() 
