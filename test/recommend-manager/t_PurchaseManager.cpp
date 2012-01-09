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
const char* TEST_DIR_STR = "recommend_test/t_PurchaseManager";
const char* PURCHASE_DB_STR = "purchase.db";
const char* CF_DIR_STR = "cf";
const char* ITEM_DB_STR = "item.db";
const char* MAX_ID_STR = "max_itemid.txt";
}

typedef map<string, set<itemid_t> > PurchaseMap;

void addPurchaseItem(
    PurchaseManager& purchaseManager,
    PurchaseMap& purchaseMap,
    const std::string& userId,
    const std::vector<itemid_t>& orderItemVec
)
{
    BOOST_CHECK(purchaseManager.addPurchaseItem(userId, orderItemVec));

    set<itemid_t>& itemIdSet = purchaseMap[userId];
    for (unsigned int i = 0; i < orderItemVec.size(); ++i)
    {
        itemIdSet.insert(orderItemVec[i]);
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

    unsigned int iterNum = 0;
    for (PurchaseManager::SDBIterator purchaseIt = purchaseManager.begin();
        purchaseIt != purchaseManager.end(); ++purchaseIt)
    {
        const string& userId = purchaseIt->first;
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

    bfs::path cfPath(bfs::path(TEST_DIR_STR) / CF_DIR_STR);
    string cfPathStr = cfPath.string();
    bfs::create_directories(cfPath);
    ItemCFManager itemCFManager(cfPathStr + "/covisit", 1000,
                                cfPathStr + "/sim", 1000,
                                cfPathStr + "/nb", 30);
    {
        BOOST_TEST_MESSAGE("add purchase...");

        PurchaseManager purchaseManager(purchasePath.string(), itemCFManager);
        std::vector<itemid_t> orderItemVec;

        orderItemVec.push_back(20);
        orderItemVec.push_back(10);
        orderItemVec.push_back(40);
        addPurchaseItem(purchaseManager, purchaseMap, "1", orderItemVec);

        orderItemVec.clear();
        orderItemVec.push_back(10);
        addPurchaseItem(purchaseManager, purchaseMap, "1", orderItemVec);

        orderItemVec.clear();
        orderItemVec.push_back(20);
        orderItemVec.push_back(30);
        addPurchaseItem(purchaseManager, purchaseMap, "2", orderItemVec);

        orderItemVec.clear();
        orderItemVec.push_back(30);
        addPurchaseItem(purchaseManager, purchaseMap, "1", orderItemVec);

        orderItemVec.clear();
        orderItemVec.push_back(30);
        orderItemVec.push_back(20);
        addPurchaseItem(purchaseManager, purchaseMap, "3", orderItemVec);

        orderItemVec.clear();
        orderItemVec.push_back(10);
        addPurchaseItem(purchaseManager, purchaseMap, "1", orderItemVec);

        orderItemVec.clear();
        orderItemVec.push_back(20);
        addPurchaseItem(purchaseManager, purchaseMap, "2", orderItemVec);

        orderItemVec.clear();
        orderItemVec.push_back(30);
        addPurchaseItem(purchaseManager, purchaseMap, "3", orderItemVec);

        checkPurchaseManager(purchaseMap, purchaseManager);
        iteratePurchaseManager(purchaseMap, purchaseManager);

        purchaseManager.flush();
    }

    {
        BOOST_TEST_MESSAGE("continue add purchase...");

        PurchaseManager purchaseManager(purchasePath.string(), itemCFManager);
        checkPurchaseManager(purchaseMap, purchaseManager);
        iteratePurchaseManager(purchaseMap, purchaseManager);

        std::vector<itemid_t> orderItemVec;

        orderItemVec.push_back(40);
        addPurchaseItem(purchaseManager, purchaseMap, "1", orderItemVec);

        orderItemVec.clear();
        orderItemVec.push_back(40);
        addPurchaseItem(purchaseManager, purchaseMap, "2", orderItemVec);

        orderItemVec.clear();
        orderItemVec.push_back(40);
        addPurchaseItem(purchaseManager, purchaseMap, "3", orderItemVec);

        orderItemVec.clear();
        orderItemVec.push_back(40);
        addPurchaseItem(purchaseManager, purchaseMap, "4", orderItemVec);

        checkPurchaseManager(purchaseMap, purchaseManager);
        iteratePurchaseManager(purchaseMap, purchaseManager);

        purchaseManager.flush();
    }
}

BOOST_AUTO_TEST_SUITE_END() 
