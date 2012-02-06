#include "EventManagerTestFixture.h"
#include <recommend-manager/storage/EventManager.h>

#include <boost/test/unit_test.hpp>
#include <cstdlib> // rand()

namespace sf1r
{

void EventManagerTestFixture::resetInstance()
{
    // flush first
    eventManager_.reset();
    eventManager_.reset(factory_->createEventManager());
}

void EventManagerTestFixture::addEvent(
    const std::string& eventStr,
    const std::string& userId,
    itemid_t itemId
)
{
    BOOST_CHECK(eventManager_->addEvent(eventStr, userId, itemId));

    userEventMap_[userId][eventStr].insert(itemId);
}

void EventManagerTestFixture::removeEvent(
    const std::string& eventStr,
    const std::string& userId,
    itemid_t itemId
)
{
    EventItemMap& eventItemMap = userEventMap_[userId];
    EventItemMap::iterator it = eventItemMap.find(eventStr);
    bool result = false;

    if (it != eventItemMap.end())
    {
        result = it->second.erase(itemId);
    }

    BOOST_CHECK_EQUAL(eventManager_->removeEvent(eventStr, userId, itemId), result);
}

void EventManagerTestFixture::addRandEvent(
    const std::string& eventStr,
    const std::string& userId,
    int itemCount
)
{
    for (int i = 0; i < itemCount; ++i)
    {
        itemid_t itemId = std::rand();

        addEvent(eventStr, userId, itemId);
    }
}

void EventManagerTestFixture::checkEventManager() const
{
    for (UserEventMap::const_iterator it = userEventMap_.begin();
        it != userEventMap_.end(); ++it)
    {
        const std::string& userId = it->first;
        const EventItemMap& goldEventMap = it->second;

        EventManager::EventItemMap eventMap;
        BOOST_CHECK(eventManager_->getEvent(userId, eventMap));

        // it might cause test result of "less" by removeEvent()
        BOOST_CHECK_LE(eventMap.size(), goldEventMap.size());

        for (EventItemMap::const_iterator eventIt = goldEventMap.begin();
            eventIt != goldEventMap.end(); ++eventIt)
        {
            const std::string& eventStr = eventIt->first;
            const std::set<itemid_t> eventItems = eventIt->second;

            BOOST_CHECK_EQUAL_COLLECTIONS(eventMap[eventStr].begin(), eventMap[eventStr].end(),
                                          eventItems.begin(), eventItems.end());
        }
    }
}

} // namespace sf1r
