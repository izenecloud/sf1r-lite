///
/// @file t_PurchaseManager.cpp
/// @brief test PurchaseManager in visit operations
/// @author Jun Jiang
/// @date Created 2011-04-19
///

#include <util/ustring/UString.h>
#include <recommend-manager/PurchaseManager.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include <string>
#include <vector>
#include <map>
#include <set>

using namespace std;
using namespace boost;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
const char* TEST_DIR_STR = "recommend_test";
const char* PURCHASE_DB_STR = "visit.db";
}

typedef map<userid_t, Purchase> PurchaseMap;

void addPurchaseItem(
    PurchaseManager& purchaseManager,
    PurchaseMap& purchaseMap,
    userid_t userId,
    itemid_t itemId,
    double price,
    int quantity,
    const std::string& orderIdStr
)
{
    BOOST_CHECK(purchaseManager.addPurchaseItem(userId, itemId, price, quantity, orderIdStr));

    Purchase& purchase = purchaseMap[userId];
    purchase.itemIdSet_.insert(itemId);

    OrderVec& orderVec = purchase.orderVec_;

    OrderItem orderItem;
    orderItem.itemId_ = itemId;
    orderItem.price_ = price;
    orderItem.quantity_ = quantity;

    if (orderIdStr.empty())
    {
        Order order;
        order.orderIdStr_ = orderIdStr;
        order.orderItemVec_.push_back(orderItem);
        orderVec.push_back(order);
    }
    else
    {
        bool isFind = false;
        for (unsigned int i = 0; i < orderVec.size(); ++i)
        {
            Order& order = orderVec[i];
            if (order.orderIdStr_ == orderIdStr)
            {
                order.orderItemVec_.push_back(orderItem);
                isFind = true;
                break;
            }
        }

        if (!isFind)
        {
            Order order;
            order.orderIdStr_ = orderIdStr;
            order.orderItemVec_.push_back(orderItem);
            orderVec.push_back(order);
        }
    }
}

void checkPurchase(const ItemIdSet& itemIdSet, const OrderVec& orderVec, const Purchase& purchase)
{
    BOOST_CHECK_EQUAL_COLLECTIONS(itemIdSet.begin(), itemIdSet.end(),
                                  purchase.itemIdSet_.begin(), purchase.itemIdSet_.end());

    BOOST_CHECK_EQUAL(orderVec.size(), purchase.orderVec_.size());
    for (unsigned int i = 0; i < orderVec.size(); ++i)
    {
        const Order& order1 = orderVec[i];
        const Order& order2 = purchase.orderVec_[i];
        BOOST_CHECK_EQUAL(order1.orderIdStr_, order2.orderIdStr_);

        BOOST_CHECK_EQUAL(order1.orderItemVec_.size(), order2.orderItemVec_.size());
        for (unsigned int j = 0; j < order1.orderItemVec_.size(); ++j)
        {
            const OrderItem& orderItem1 = order1.orderItemVec_[j];
            const OrderItem& orderItem2 = order2.orderItemVec_[j];
            BOOST_CHECK_EQUAL(orderItem1.itemId_, orderItem2.itemId_);
            BOOST_CHECK_EQUAL(orderItem1.price_, orderItem2.price_);
            BOOST_CHECK_EQUAL(orderItem1.quantity_, orderItem2.quantity_);
        }
    }
}

void checkPurchaseManager(const PurchaseMap& purchaseMap, PurchaseManager& purchaseManager)
{
    BOOST_CHECK_EQUAL(purchaseManager.purchaseUserNum(), purchaseMap.size());

    for (PurchaseMap::const_iterator it = purchaseMap.begin();
        it != purchaseMap.end(); ++it)
    {
        ItemIdSet itemIdSet;
        OrderVec orderVec;
        BOOST_CHECK(purchaseManager.getPurchaseItemSet(it->first, itemIdSet));
        BOOST_CHECK(purchaseManager.getPurchaseOrderVec(it->first, orderVec));
        checkPurchase(itemIdSet, orderVec, it->second);
    }
}

void iteratePurchaseManager(const PurchaseMap& purchaseMap, PurchaseManager& purchaseManager)
{
    BOOST_CHECK_EQUAL(purchaseManager.purchaseUserNum(), purchaseMap.size());

    int iterNum = 0;
    for (PurchaseManager::SDBIterator purchaseIt = purchaseManager.begin();
        purchaseIt != purchaseManager.end(); ++purchaseIt)
    {
        userid_t userId = purchaseIt->first;
        const Purchase& purchase = purchaseIt->second;

        PurchaseMap::const_iterator it = purchaseMap.find(userId);
        BOOST_CHECK(it != purchaseMap.end());
        checkPurchase(purchase.itemIdSet_, purchase.orderVec_, purchase);
        ++iterNum;
    }

    BOOST_TEST_MESSAGE("iterNum: " << iterNum);
    BOOST_CHECK_EQUAL(iterNum, purchaseManager.purchaseUserNum());
}

BOOST_AUTO_TEST_SUITE(PurchaseManagerTest)

BOOST_AUTO_TEST_CASE(checkPurchase)
{
    bfs::path purchasePath(bfs::path(TEST_DIR_STR) / PURCHASE_DB_STR);
    boost::filesystem::remove(purchasePath);
    bfs::create_directories(TEST_DIR_STR);

    PurchaseMap purchaseMap;

    {
        BOOST_TEST_MESSAGE("add purchase...");

        PurchaseManager purchaseManager(purchasePath.string());
        addPurchaseItem(purchaseManager, purchaseMap, 1, 10, 50, 3, "order1_10");
        addPurchaseItem(purchaseManager, purchaseMap, 1, 40, 1000, 1, "");
        addPurchaseItem(purchaseManager, purchaseMap, 2, 10, 1, 7, "");
        addPurchaseItem(purchaseManager, purchaseMap, 2, 20, 20, 2, "order2_20");
        addPurchaseItem(purchaseManager, purchaseMap, 2, 30, 200, 8, "order2_20");
        addPurchaseItem(purchaseManager, purchaseMap, 1, 20, 13.5, 1, "order1_10");
        addPurchaseItem(purchaseManager, purchaseMap, 1, 30, 100, 10, "order1_30");
        addPurchaseItem(purchaseManager, purchaseMap, 3, 30, 500, 4, "order3_30");
        addPurchaseItem(purchaseManager, purchaseMap, 3, 20, 22.2, 9, "");
        addPurchaseItem(purchaseManager, purchaseMap, 1, 10, 50, 3, "");
        addPurchaseItem(purchaseManager, purchaseMap, 2, 20, 20, 2, "");
        addPurchaseItem(purchaseManager, purchaseMap, 3, 30, 500, 4, "");

        checkPurchaseManager(purchaseMap, purchaseManager);
        iteratePurchaseManager(purchaseMap, purchaseManager);

        purchaseManager.flush();
    }

    {
        BOOST_TEST_MESSAGE("continue add purchase...");

        PurchaseManager purchaseManager(purchasePath.string());
        checkPurchaseManager(purchaseMap, purchaseManager);
        iteratePurchaseManager(purchaseMap, purchaseManager);

        addPurchaseItem(purchaseManager, purchaseMap, 1, 40, 50, 3, "order1_40");
        addPurchaseItem(purchaseManager, purchaseMap, 2, 40, 20, 2, "");
        addPurchaseItem(purchaseManager, purchaseMap, 3, 40, 500, 4, "");
        addPurchaseItem(purchaseManager, purchaseMap, 4, 40, 34, 5, "order4_40");

        checkPurchaseManager(purchaseMap, purchaseManager);
        iteratePurchaseManager(purchaseMap, purchaseManager);

        purchaseManager.flush();
    }
}

BOOST_AUTO_TEST_SUITE_END() 
