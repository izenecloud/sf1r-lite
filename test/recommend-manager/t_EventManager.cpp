/// @file t_EventManager.cpp
/// @brief test EventManager in user event
/// @author Jun Jiang
/// @date Created 2011-08-09
///

#include <recommend-manager/EventManager.h>

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
const char* TEST_DIR_STR = "recommend_test/t_EventManager";
const char* EVENT_DB_STR = "event.db";
}

typedef map<string, set<itemid_t> > EventItemMap;
typedef map<userid_t, EventItemMap > UserEventMap;

void updateEvent(
    bool isAdd,
    EventManager& eventManager,
    UserEventMap& userEventMap,
    const string& eventStr,
    userid_t userId,
    itemid_t itemId
)
{
    if (isAdd)
    {
        BOOST_CHECK(eventManager.addEvent(eventStr, userId, itemId));
        userEventMap[userId][eventStr].insert(itemId);
    }
    else
    {
        if (userEventMap[userId][eventStr].erase(itemId))
        {
            BOOST_CHECK(eventManager.removeEvent(eventStr, userId, itemId));
        }
        else
        {
            BOOST_CHECK(eventManager.removeEvent(eventStr, userId, itemId) == false);
        }
    }
}

void checkEventManager(const UserEventMap& userEventMap, EventManager& eventManager)
{
    for (UserEventMap::const_iterator it = userEventMap.begin();
        it != userEventMap.end(); ++it)
    {
        EventManager::EventItemMap eventMap;
        BOOST_CHECK(eventManager.getEvent(it->first, eventMap));

        // it might cause "less" relation by "erase" in updateEvent
        BOOST_CHECK_LE(eventMap.size(), it->second.size());

        for (EventItemMap::const_iterator eventIt = it->second.begin();
            eventIt != it->second.end(); ++eventIt)
        {
            BOOST_CHECK_EQUAL_COLLECTIONS(eventMap[eventIt->first].begin(), eventMap[eventIt->first].end(),
                                          eventIt->second.begin(), eventIt->second.end());
        }
    }
}

BOOST_AUTO_TEST_SUITE(EventManagerTest)

BOOST_AUTO_TEST_CASE(checkEvent)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

    bfs::path eventPath(bfs::path(TEST_DIR_STR) / EVENT_DB_STR);
    bfs::create_directories(TEST_DIR_STR);

    UserEventMap userEventMap;

    {
        BOOST_TEST_MESSAGE("check empty event...");
        EventManager eventManager(eventPath.string());

        BOOST_CHECK(userEventMap[1].empty());
        BOOST_CHECK(userEventMap[10].empty());
        BOOST_CHECK(userEventMap[100].empty());

        checkEventManager(userEventMap, eventManager);
    }

    {
        BOOST_TEST_MESSAGE("update event...");
        EventManager eventManager(eventPath.string());

        updateEvent(true, eventManager, userEventMap, "wish_list", 1, 1);
        updateEvent(true, eventManager, userEventMap, "own", 1, 2);
        updateEvent(true, eventManager, userEventMap, "like", 1, 3);
        updateEvent(true, eventManager, userEventMap, "favorite", 1, 4);
        updateEvent(false, eventManager, userEventMap, "not_rec_result", 1, 5);
        updateEvent(false, eventManager, userEventMap, "not_rec_input", 1, 6);

        updateEvent(true, eventManager, userEventMap, "not_rec_result", 2, 1);
        updateEvent(true, eventManager, userEventMap, "not_rec_input", 2, 2);

        checkEventManager(userEventMap, eventManager);
    }

    {
        BOOST_TEST_MESSAGE("continue update event...");
        EventManager eventManager(eventPath.string());

        checkEventManager(userEventMap, eventManager);

        updateEvent(false, eventManager, userEventMap, "wish_list", 1, 1);
        updateEvent(false, eventManager, userEventMap, "own", 1, 2);
        updateEvent(true, eventManager, userEventMap, "like", 1, 3);
        updateEvent(true, eventManager, userEventMap, "favorite", 1, 4);
        updateEvent(true, eventManager, userEventMap, "not_rec_result", 1, 5);
        updateEvent(true, eventManager, userEventMap, "not_rec_input", 1, 6);

        updateEvent(false, eventManager, userEventMap, "wish_list", 2, 1);
        updateEvent(false, eventManager, userEventMap, "own", 2, 2);
        updateEvent(true, eventManager, userEventMap, "like", 2, 3);
        updateEvent(true, eventManager, userEventMap, "favorite", 2, 4);
        updateEvent(false, eventManager, userEventMap, "not_rec_result", 2, 1);
        updateEvent(false, eventManager, userEventMap, "not_rec_input", 2, 2);

        checkEventManager(userEventMap, eventManager);
    }
}

BOOST_AUTO_TEST_SUITE_END() 
