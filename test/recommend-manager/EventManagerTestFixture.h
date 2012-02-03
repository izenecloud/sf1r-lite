///
/// @file EventManagerTestFixture.h
/// @brief fixture class to test EventManager
/// @author Jun Jiang
/// @date Created 2012-02-03
///

#ifndef EVENT_MANAGER_TEST_FIXTURE
#define EVENT_MANAGER_TEST_FIXTURE

#include "RecommendStorageTestFixture.h"
#include <recommend-manager/RecTypes.h>

#include <map>
#include <set>
#include <string>

namespace sf1r
{
class EventManager;

class EventManagerTestFixture : public RecommendStorageTestFixture
{
public:
    virtual void resetInstance();

    void addEvent(
        const std::string& eventStr,
        const std::string& userId,
        itemid_t itemId
    );

    void removeEvent(
        const std::string& eventStr,
        const std::string& userId,
        itemid_t itemId
    );

    void addRandEvent(
        const std::string& eventStr,
        const std::string& userId,
        int itemCount
    );

    void checkEventManager() const;

protected:
    boost::scoped_ptr<EventManager> eventManager_;

    typedef std::map<std::string, std::set<itemid_t> > EventItemMap;
    typedef std::map<std::string, EventItemMap> UserEventMap;
    UserEventMap userEventMap_;
};

} // namespace sf1r

#endif //EVENT_MANAGER_TEST_FIXTURE
