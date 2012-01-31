#include "CartManagerTestFixture.h"
#include "test_util.h"
#include <recommend-manager/CartManager.h>

#include <boost/test/unit_test.hpp>

namespace sf1r
{

CartManagerTestFixture::CartManagerTestFixture()
    : cartManager_(NULL)
{
}

void CartManagerTestFixture::setCartManager(CartManager* cartManager)
{
    cartManager_ = cartManager;
}

void CartManagerTestFixture::updateCart(const std::string& userId, const std::string& items)
{
    std::vector<itemid_t> cartItems;
    split_str_to_items(items, cartItems);

    BOOST_CHECK(cartManager_->updateCart(userId, cartItems));

    cartMap_[userId].swap(cartItems);
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
        BOOST_CHECK_EQUAL_COLLECTIONS(cartItems.begin(), cartItems.end(),
                                      it->second.begin(), it->second.end());
    }
}

} // namespace sf1r
