/// @file t_EventManager.cpp
/// @brief test EventManager in user event
/// @author Jun Jiang
/// @date Created 2011-08-09
///

#include "EventManagerTestFixture.h"
#include <recommend-manager/storage/EventManager.h>

#include <boost/test/unit_test.hpp>
#include <string>

using namespace std;
using namespace sf1r;

namespace
{
const string TEST_DIR_STR = "recommend_test/t_EventManager";
const string KEYSPACE_NAME = "test_recommend";
const string COLLECTION_NAME = "example";

void testEvent1(EventManagerTestFixture& fixture)
{
    BOOST_TEST_MESSAGE("1st update event...");

    fixture.resetInstance();

    fixture.addEvent("wish_list", "1", 1);
    fixture.addEvent("own", "1", 2);
    fixture.addEvent("like", "1", 3);
    fixture.addEvent("favorite", "1", 4);

    fixture.removeEvent("not_rec_result", "1", 5);
    fixture.removeEvent("not_rec_input", "1", 6);

    fixture.addEvent("not_rec_result", "2", 1);
    fixture.addEvent("not_rec_input", "2", 2);

    fixture.addRandEvent("wish_list", "7", 99);
    fixture.addRandEvent("own", "6", 100);
    fixture.addRandEvent("like", "5", 101);
    fixture.addRandEvent("not_rec_result", "4", 1234);

    fixture.checkEventManager();
}

void testEvent2(EventManagerTestFixture& fixture)
{
    BOOST_TEST_MESSAGE("2nd update event...");

    fixture.resetInstance();

    fixture.removeEvent("wish_list", "1", 1);
    fixture.removeEvent("own", "1", 2);

    fixture.addEvent("like", "1", 3);
    fixture.addEvent("favorite", "1", 4);
    fixture.addEvent("not_rec_result", "1", 5);
    fixture.addEvent("not_rec_input", "1", 6);

    fixture.addEvent("wish_list", "2", 1);
    fixture.addEvent("own", "2", 2);
    fixture.addEvent("like", "2", 3);
    fixture.addEvent("favorite", "2", 4);

    fixture.removeEvent("not_rec_result", "2", 1);
    fixture.removeEvent("not_rec_input", "2", 2);

    fixture.removeEvent("wish_list", "7", 1);
    fixture.removeEvent("own", "6", 10);
    fixture.removeEvent("like", "5", 100);
    fixture.removeEvent("not_rec_input", "4", 1000);

    fixture.checkEventManager();
}

}

BOOST_AUTO_TEST_SUITE(EventManagerTest)

BOOST_FIXTURE_TEST_CASE(checkLocalEventManager, EventManagerTestFixture)
{
    BOOST_REQUIRE(initLocalStorage(COLLECTION_NAME, TEST_DIR_STR));

    testEvent1(*this);
    testEvent2(*this);
}

BOOST_AUTO_TEST_SUITE_END() 
