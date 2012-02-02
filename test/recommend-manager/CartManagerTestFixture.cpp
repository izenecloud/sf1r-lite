#include "CartManagerTestFixture.h"
#include "test_util.h"
#include <recommend-manager/storage/CartManager.h>

#include <boost/test/unit_test.hpp>

namespace sf1r
{

void CartManagerTestFixture::resetInstance()
{
    // flush first
    cartManager_.reset();
    cartManager_.reset(factory_->createCartManager());
}

void CartManagerTestFixture::updateCart(const std::string& userId, const std::string& items)
{
    std::vector<itemid_t> cartItems;
    split_str_to_items(items, cartItems);

    BOOST_CHECK(cartManager_->updateCart(userId, cartItems));

    std::set<itemid_t>& itemSet = cartMap_[userId];
    itemSet.clear();
    itemSet.insert(cartItems.begin(), cartItems.end());
}

void CartManagerTestFixture::updateRandItem(const std::string& userId, int itemCount)
{
    updateCart(userId, generate_rand_items_str(itemCount));
}

void CartManagerTestFixture::checkCartManager() const
{
    for (CartMap::const_iterator it = cartMap_.begin();
        it != cartMap_.end(); ++it)
    {
        vector<itemid_t> cartItems;
        BOOST_CHECK(cartManager_->getCart(it->first, cartItems));

        std::set<itemid_t> itemSet(cartItems.begin(), cartItems.end());

        BOOST_CHECK_EQUAL_COLLECTIONS(itemSet.begin(), itemSet.end(),
                                      it->second.begin(), it->second.end());
    }
}

} // namespace sf1r
