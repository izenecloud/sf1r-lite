/// @file t_CartManager.cpp
/// @brief test CartManager in shopping cart operations
/// @author Jun Jiang
/// @date Created 2011-08-07
///

#include <recommend-manager/CartManager.h>

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
const char* TEST_DIR_STR = "recommend_test/t_CartManager";
const char* CART_DB_STR = "cart.db";
}

typedef map<userid_t, vector<itemid_t> > CartMap;

void updateCart(
    CartManager& cartManager,
    CartMap& cartMap,
    userid_t userId,
    const vector<itemid_t>& cartItemVec
)
{
    BOOST_CHECK(cartManager.updateCart(userId, cartItemVec));

    cartMap[userId] = cartItemVec;
}

void checkCartManager(const CartMap& cartMap, CartManager& cartManager)
{
    for (CartMap::const_iterator it = cartMap.begin();
        it != cartMap.end(); ++it)
    {
        vector<itemid_t> cartItemVec;
        BOOST_CHECK(cartManager.getCart(it->first, cartItemVec));
        BOOST_CHECK_EQUAL_COLLECTIONS(cartItemVec.begin(), cartItemVec.end(),
                                      it->second.begin(), it->second.end());
    }
}

BOOST_AUTO_TEST_SUITE(CartManagerTest)

BOOST_AUTO_TEST_CASE(checkCart)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

    bfs::path cartPath(bfs::path(TEST_DIR_STR) / CART_DB_STR);
    bfs::create_directories(TEST_DIR_STR);

    CartMap cartMap;

    {
        BOOST_TEST_MESSAGE("check empty cart...");
        CartManager cartManager(cartPath.string());

        BOOST_CHECK(cartMap[1].empty());
        BOOST_CHECK(cartMap[10].empty());
        BOOST_CHECK(cartMap[100].empty());

        checkCartManager(cartMap, cartManager);
    }

    {
        BOOST_TEST_MESSAGE("update cart...");
        CartManager cartManager(cartPath.string());

        vector<itemid_t> cartItemVec;

        cartItemVec.push_back(2);
        cartItemVec.push_back(4);
        cartItemVec.push_back(6);
        updateCart(cartManager, cartMap, 1, cartItemVec);

        cartItemVec.clear();
        cartItemVec.push_back(11);
        updateCart(cartManager, cartMap, 10, cartItemVec);

        cartItemVec.clear();
        cartItemVec.push_back(102);
        cartItemVec.push_back(103);
        updateCart(cartManager, cartMap, 100, cartItemVec);

        checkCartManager(cartMap, cartManager);
    }

    {
        BOOST_TEST_MESSAGE("continue update cart...");
        CartManager cartManager(cartPath.string());

        checkCartManager(cartMap, cartManager);

        vector<itemid_t> cartItemVec;

        cartItemVec.push_back(1);
        cartItemVec.push_back(3);
        cartItemVec.push_back(5);
        cartItemVec.push_back(7);
        cartItemVec.push_back(9);
        updateCart(cartManager, cartMap, 1, cartItemVec);

        cartItemVec.clear();
        updateCart(cartManager, cartMap, 10, cartItemVec);

        cartItemVec.clear();
        updateCart(cartManager, cartMap, 100, cartItemVec);

        cartItemVec.clear();
        cartItemVec.push_back(1001);
        cartItemVec.push_back(1010);
        cartItemVec.push_back(1100);
        updateCart(cartManager, cartMap, 1000, cartItemVec);

        cartItemVec.clear();
        cartItemVec.push_back(10003);
        cartItemVec.push_back(10004);
        cartItemVec.push_back(10005);
        cartItemVec.push_back(10006);
        updateCart(cartManager, cartMap, 10000, cartItemVec);

        checkCartManager(cartMap, cartManager);
    }
}

BOOST_AUTO_TEST_SUITE_END() 
