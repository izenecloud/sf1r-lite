///
/// @file t_ItemManager.cpp
/// @brief test ItemManager in Item operations
/// @author Jun Jiang
/// @date Created 2011-04-19
///

#include <util/ustring/UString.h>
#include <recommend-manager/ItemManager.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include <string>
#include <vector>

using namespace std;
using namespace boost;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
const char* TEST_DIR_STR = "recommend_test";
const char* ITEM_DB_STR = "item.db";
const char* MAX_ID_STR = "max_itemid.txt";
}

void checkItem(const Item& item1, const Item& item2)
{
    BOOST_CHECK_EQUAL(item1.idStr_, item2.idStr_);

    Item::PropValueMap::const_iterator it1 = item1.propValueMap_.begin();
    Item::PropValueMap::const_iterator it2 = item2.propValueMap_.begin();
    for (; it1 != item1.propValueMap_.end() && it2 != item2.propValueMap_.end(); ++it1, ++it2)
    {
        BOOST_CHECK_EQUAL(it1->second, it2->second);
    }

    BOOST_CHECK(it1 == item1.propValueMap_.end());
    BOOST_CHECK(it2 == item2.propValueMap_.end());
}

void checkItemManager(const vector<itemid_t>& idVec, const vector<Item>& itemVec, ItemManager& itemManager)
{
    BOOST_CHECK_EQUAL(idVec.size(), itemVec.size());
    BOOST_CHECK_EQUAL(itemManager.itemNum(), itemVec.size());

    Item item2;
    for (size_t i = 0; i < idVec.size(); ++i)
    {
        BOOST_CHECK(itemManager.getItem(idVec[i], item2));
        checkItem(itemVec[i], item2);
    }
}

void iterateItemManager(const vector<itemid_t>& idVec, const vector<Item>& itemVec, ItemManager& itemManager)
{
    BOOST_CHECK_EQUAL(idVec.size(), itemVec.size());
    BOOST_CHECK_EQUAL(itemManager.itemNum(), itemVec.size());

    int iterNum = 0;
    for (ItemManager::SDBIterator itemIt = itemManager.begin();
        itemIt != itemManager.end(); ++itemIt)
    {
        bool isFind = false;
        for (size_t i = 0; i < idVec.size(); ++i)
        {
            if (idVec[i] == itemIt->first)
            {
                checkItem(itemIt->second, itemVec[i]);
                isFind = true;
                break;
            }
        }
        BOOST_CHECK(isFind);
        ++iterNum;
    }

    BOOST_TEST_MESSAGE("iterNum: " << iterNum);
    BOOST_CHECK_EQUAL(iterNum, itemManager.itemNum());
}

BOOST_AUTO_TEST_SUITE(ItemManagerTest)

BOOST_AUTO_TEST_CASE(checkItem)
{
    bfs::path itemPath(bfs::path(TEST_DIR_STR) / ITEM_DB_STR);
    bfs::path maxIdPath(bfs::path(TEST_DIR_STR) / MAX_ID_STR);
    boost::filesystem::remove(itemPath);
    boost::filesystem::remove(maxIdPath);
    bfs::create_directories(TEST_DIR_STR);

    vector<itemid_t> idVec;
    vector<Item> itemVec;

    idVec.push_back(1);
    itemVec.push_back(Item());
    Item& item1 = itemVec.back();
    item1.idStr_ = "aaa_1";
    item1.propValueMap_["name"].assign("商品1", ENCODING_TYPE);
    item1.propValueMap_["link"].assign("http://www.e-commerce.com/item1", ENCODING_TYPE);
    item1.propValueMap_["price"].assign("23.5", ENCODING_TYPE);
    item1.propValueMap_["image"].assign("http://www.e-commerce.com/item1/image", ENCODING_TYPE);

    idVec.push_back(2);
    itemVec.push_back(Item());
    Item& item2 = itemVec.back();
    item2.idStr_ = "aaa_51";
    item2.propValueMap_["name"].assign("商品2", ENCODING_TYPE);
    item2.propValueMap_["link"].assign("http://www.e-commerce.com/item51", ENCODING_TYPE);
    item2.propValueMap_["price"].assign("80", ENCODING_TYPE);
    item2.propValueMap_["image"].assign("http://www.e-commerce.com/item51/image", ENCODING_TYPE);

    itemid_t maxItemId = idVec.back();

    {
        BOOST_TEST_MESSAGE("add item...");
        ItemManager itemManager(itemPath.string(), maxIdPath.string());
        for (size_t i = 0; i < itemVec.size(); ++i)
        {
            BOOST_CHECK(itemManager.addItem(idVec[i], itemVec[i]));
        }
        // duplicate add should fail
        BOOST_CHECK(itemManager.addItem(idVec[0], itemVec[0]) == false);

        checkItemManager(idVec, itemVec, itemManager);
        iterateItemManager(idVec, itemVec, itemManager);

        BOOST_CHECK_EQUAL(itemManager.maxItemId(), maxItemId);

        itemManager.flush();
    }

    {
        BOOST_TEST_MESSAGE("update item...");
        ItemManager itemManager(itemPath.string(), maxIdPath.string());

        checkItemManager(idVec, itemVec, itemManager);

        BOOST_CHECK_EQUAL(itemManager.maxItemId(), maxItemId);

        Item& item2 = itemVec.back();
        item2.propValueMap_["name"].assign("商品22222", ENCODING_TYPE);
        item2.propValueMap_["link"].assign("http://www.e-commerce.com/item22222", ENCODING_TYPE);
        item2.propValueMap_["price"].assign("2180", ENCODING_TYPE);
        item2.propValueMap_["image"].assign("http://www.e-commerce.com/item22222/image", ENCODING_TYPE);

        BOOST_CHECK(itemManager.updateItem(idVec.back(), item2));
        checkItemManager(idVec, itemVec, itemManager);
        iterateItemManager(idVec, itemVec, itemManager);

        BOOST_TEST_MESSAGE("remove item...");
        BOOST_CHECK(itemManager.hasItem(idVec.front()));
        BOOST_CHECK(itemManager.removeItem(idVec.front()));
        BOOST_CHECK(itemManager.hasItem(idVec.front()) == false);
        itemVec.erase(itemVec.begin());
        idVec.erase(idVec.begin());
        checkItemManager(idVec, itemVec, itemManager);
        iterateItemManager(idVec, itemVec, itemManager);

        BOOST_CHECK_EQUAL(itemManager.maxItemId(), maxItemId);

        itemManager.flush();
    }

    {
        BOOST_TEST_MESSAGE("empty item...");
        ItemManager itemManager(itemPath.string(), maxIdPath.string());

        BOOST_CHECK_EQUAL(itemManager.maxItemId(), maxItemId);

        checkItemManager(idVec, itemVec, itemManager);
        iterateItemManager(idVec, itemVec, itemManager);

        BOOST_CHECK(itemManager.removeItem(idVec.front()));
        itemVec.erase(itemVec.begin());
        idVec.erase(idVec.begin());
        checkItemManager(idVec, itemVec, itemManager);
        iterateItemManager(idVec, itemVec, itemManager);

        BOOST_CHECK_EQUAL(itemManager.maxItemId(), maxItemId);

        itemManager.flush();
    }
}

BOOST_AUTO_TEST_SUITE_END() 
