///
/// @file RateManagerTestFixture.h
/// @brief fixture class to test RateManager
/// @author Jun Jiang
/// @date Created 2012-02-03
///

#ifndef RATE_MANAGER_TEST_FIXTURE
#define RATE_MANAGER_TEST_FIXTURE

#include "RecommendStorageTestFixture.h"
#include <recommend-manager/common/RecTypes.h>

#include <map>
#include <string>

namespace sf1r
{
class RateManager;

class RateManagerTestFixture : public RecommendStorageTestFixture
{
public:
    virtual void resetInstance();

    void addRate(
        const string& userId,
        itemid_t itemId,
        rate_t rate
    );

    void removeRate(
        const string& userId,
        itemid_t itemId
    );

    void addRandRate(
        const std::string& userId,
        int itemCount
    );

    void checkRateManager() const;

protected:
    boost::scoped_ptr<RateManager> rateManager_;

    typedef std::map<std::string, ItemRateMap> UserRateMap;
    UserRateMap userRateMap_;
};

} // namespace sf1r

#endif //RATE_MANAGER_TEST_FIXTURE
