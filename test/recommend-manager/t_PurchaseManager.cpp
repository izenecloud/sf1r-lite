/// @file t_PurchaseManager.cpp
/// @brief test PurchaseManager in visit operations
/// @author Jun Jiang
/// @date Created 2011-04-19
///

#include <util/ustring/UString.h>
#include <recommend-manager/PurchaseManager.h>
#include <recommend-manager/ItemManager.h>
#include <common/JobScheduler.h>

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
const char* TEST_DIR_STR = "recommend_test/t_PurchaseManager";
const char* PURCHASE_DB_STR = "purchase.db";
const char* CF_DIR_STR = "cf";
const char* ITEM_DB_STR = "item.db";
const char* MAX_ID_STR = "max_itemid.txt";
}

typedef map<userid_t, set<itemid_t> > PurchaseMap;
typedef PurchaseManager::OrderItem OrderItem;
typedef PurchaseManager::OrderItemVec OrderItemVec;

void addPurchaseItem(
    PurchaseManager& purchaseManager,
    PurchaseMap& purchaseMap,
    userid_t userId,
    const OrderItemVec& orderItemVec,
    const std::string& orderIdStr
)
{
    BOOST_CHECK(purchaseManager.addPurchaseItem(userId, orderItemVec, orderIdStr));

    set<itemid_t>& itemIdSet = purchaseMap[userId];
    for (unsigned int i = 0; i < orderItemVec.size(); ++i)
    {
        itemIdSet.insert(orderItemVec[i].itemId_);
    }
}

void checkPurchaseManager(const PurchaseMap& purchaseMap, PurchaseManager& purchaseManager)
{
    BOOST_CHECK_EQUAL(purchaseManager.purchaseUserNum(), purchaseMap.size());

    for (PurchaseMap::const_iterator it = purchaseMap.begin();
        it != purchaseMap.end(); ++it)
    {
        ItemIdSet itemIdSet;
        BOOST_CHECK(purchaseManager.getPurchaseItemSet(it->first, itemIdSet));
        BOOST_CHECK_EQUAL_COLLECTIONS(itemIdSet.begin(), itemIdSet.end(),
                                      it->second.begin(), it->second.end());
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
        const ItemIdSet& itemIdSet = purchaseIt->second;

        PurchaseMap::const_iterator it = purchaseMap.find(userId);
        BOOST_CHECK(it != purchaseMap.end());
        BOOST_CHECK_EQUAL_COLLECTIONS(itemIdSet.begin(), itemIdSet.end(),
                                      it->second.begin(), it->second.end());
        ++iterNum;
    }

    BOOST_TEST_MESSAGE("iterNum: " << iterNum);
    BOOST_CHECK_EQUAL(iterNum, purchaseManager.purchaseUserNum());
}

BOOST_AUTO_TEST_SUITE(PurchaseManagerTest)

BOOST_AUTO_TEST_CASE(checkPurchase)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

    bfs::path purchasePath(bfs::path(TEST_DIR_STR) / PURCHASE_DB_STR);
    bfs::path itemPath(bfs::path(TEST_DIR_STR) / ITEM_DB_STR);
    bfs::path maxIdPath(bfs::path(TEST_DIR_STR) / MAX_ID_STR);

    bfs::create_directories(TEST_DIR_STR);

    PurchaseMap purchaseMap;

    JobScheduler* jobScheduler = new JobScheduler();
    bfs::path cfPath(bfs::path(TEST_DIR_STR) / CF_DIR_STR);
    string cfPathStr = cfPath.string();
    bfs::create_directories(cfPath);
    ItemCFManager* itemCFManager = new ItemCFManager(cfPathStr + "/covisit", 1000,
                                                     cfPathStr + "/sim", 1000,
                                                     cfPathStr + "/nb", 30,
                                                     cfPathStr + "/rec", 1000);

    // add items first
    ItemManager* itemManager = new ItemManager(itemPath.string(), maxIdPath.string());
    const itemid_t MAX_ITEM_ID = 50;
    for (itemid_t i = 1; i <= MAX_ITEM_ID; ++i)
    {
        BOOST_CHECK(itemManager->addItem(i, Item()));
    }

    {
        BOOST_TEST_MESSAGE("add purchase...");

        PurchaseManager purchaseManager(purchasePath.string(), jobScheduler, itemCFManager, itemManager);
        OrderItemVec orderItemVec;

        orderItemVec.push_back(OrderItem(20, 5, 13.5));
        orderItemVec.push_back(OrderItem(10, 3, 50));
        orderItemVec.push_back(OrderItem(40, 1, 1000));
        addPurchaseItem(purchaseManager, purchaseMap, 1, orderItemVec, "order1_10");

        orderItemVec.clear();
        orderItemVec.push_back(OrderItem(10, 1, 7.0));
        addPurchaseItem(purchaseManager, purchaseMap, 1, orderItemVec, "");

        orderItemVec.clear();
        orderItemVec.push_back(OrderItem(20, 10, 20.0));
        orderItemVec.push_back(OrderItem(30, 8, 200.0));
        addPurchaseItem(purchaseManager, purchaseMap, 2, orderItemVec, "order2_20");

        orderItemVec.clear();
        orderItemVec.push_back(OrderItem(30, 10, 100));
        addPurchaseItem(purchaseManager, purchaseMap, 1, orderItemVec, "order1_30");

        orderItemVec.clear();
        orderItemVec.push_back(OrderItem(30, 4, 500));
        orderItemVec.push_back(OrderItem(20, 9, 22.2));
        addPurchaseItem(purchaseManager, purchaseMap, 3, orderItemVec, "order3_30");

        orderItemVec.clear();
        orderItemVec.push_back(OrderItem(10, 3, 50));
        addPurchaseItem(purchaseManager, purchaseMap, 1, orderItemVec, "");

        orderItemVec.clear();
        orderItemVec.push_back(OrderItem(20, 2, 20));
        addPurchaseItem(purchaseManager, purchaseMap, 2, orderItemVec, "");

        orderItemVec.clear();
        orderItemVec.push_back(OrderItem(30, 4, 500));
        addPurchaseItem(purchaseManager, purchaseMap, 3, orderItemVec, "");

        checkPurchaseManager(purchaseMap, purchaseManager);
        iteratePurchaseManager(purchaseMap, purchaseManager);

        purchaseManager.flush();
    }

    {
        BOOST_TEST_MESSAGE("continue add purchase...");

        PurchaseManager purchaseManager(purchasePath.string(), jobScheduler, itemCFManager, itemManager);
        checkPurchaseManager(purchaseMap, purchaseManager);
        iteratePurchaseManager(purchaseMap, purchaseManager);

        OrderItemVec orderItemVec;

        orderItemVec.push_back(OrderItem(40, 3, 50));
        addPurchaseItem(purchaseManager, purchaseMap, 1, orderItemVec, "order1_40");

        orderItemVec.clear();
        orderItemVec.push_back(OrderItem(40, 2, 20));
        addPurchaseItem(purchaseManager, purchaseMap, 2, orderItemVec, "");

        orderItemVec.clear();
        orderItemVec.push_back(OrderItem(40, 4, 500));
        addPurchaseItem(purchaseManager, purchaseMap, 3, orderItemVec, "");

        orderItemVec.clear();
        orderItemVec.push_back(OrderItem(40, 5, 34));
        addPurchaseItem(purchaseManager, purchaseMap, 4, orderItemVec, "order4_40");

        checkPurchaseManager(purchaseMap, purchaseManager);
        iteratePurchaseManager(purchaseMap, purchaseManager);

        purchaseManager.flush();
    }

    delete jobScheduler;
    delete itemCFManager;
    delete itemManager;
}

BOOST_AUTO_TEST_SUITE_END() 
