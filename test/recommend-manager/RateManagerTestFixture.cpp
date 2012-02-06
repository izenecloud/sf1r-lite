#include "RateManagerTestFixture.h"
#include <recommend-manager/storage/RateManager.h>

#include <boost/test/unit_test.hpp>
#include <cstdlib> // rand()

namespace sf1r
{

void RateManagerTestFixture::resetInstance()
{
    // flush first
    rateManager_.reset();
    rateManager_.reset(factory_->createRateManager());
}

void RateManagerTestFixture::addRate(
    const string& userId,
    itemid_t itemId,
    rate_t rate
)
{
    BOOST_CHECK(rateManager_->addRate(userId, itemId, rate));

    userRateMap_[userId][itemId] = rate;
}

void RateManagerTestFixture::removeRate(
    const string& userId,
    itemid_t itemId
)
{
    bool result = userRateMap_[userId].erase(itemId);

    BOOST_CHECK_EQUAL(rateManager_->removeRate(userId, itemId), result);
}

void RateManagerTestFixture::addRandRate(
    const std::string& userId,
    int itemCount
)
{
    for (int i = 0; i < itemCount; ++i)
    {
        itemid_t itemId = std::rand();
        rate_t rate = std::rand()%5 + 1; // range [1, 5]

        addRate(userId, itemId, rate);
    }
}

void RateManagerTestFixture::checkRateManager() const
{
    for (UserRateMap::const_iterator it = userRateMap_.begin();
        it != userRateMap_.end(); ++it)
    {
        ItemRateMap itemRateMap;
        BOOST_CHECK(rateManager_->getItemRateMap(it->first, itemRateMap));

        const ItemRateMap& goldItemRateMap = it->second;
        BOOST_CHECK_EQUAL(itemRateMap.size(), goldItemRateMap.size());

        for (ItemRateMap::const_iterator rateIt = goldItemRateMap.begin();
            rateIt != goldItemRateMap.end(); ++rateIt)
        {
            BOOST_CHECK_EQUAL(itemRateMap[rateIt->first], rateIt->second);
        }
    }
}

} // namespace sf1r
