/// @file t_OrderManager.cpp
/// @brief test OrderManager

#include <util/ustring/UString.h>
#include <recommend-manager/OrderManager.h>
#include <recommend-manager/ItemManager.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>
#include <iterator>
#include <sstream>

using namespace std;
using namespace boost;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
const char* ORDER_HOME_STR = "recommend_test/t_OrderManager";
const char* ITEM_DB_STR = "item.db";
const char* MAX_ID_STR = "max_itemid.txt";
ostream_iterator<itemid_t> COUT_ITER(cout, ",");

template <class OutputIterator>
void splitItems(const char* inputStr, OutputIterator result)
{
    uint32_t itemId;
    stringstream ss(inputStr);
    while (ss >> itemId)
    {
        *result++ = itemId;
    }
}

void checkGetFreqItemSets(OrderManager& orderManager, const char* inputItemStr, const char* goldItemStr)
{
    std::list<itemid_t> inputItems;
    vector<itemid_t> goldResult;
    splitItems(inputItemStr, back_insert_iterator< list<itemid_t> >(inputItems));
    splitItems(goldItemStr, back_insert_iterator< vector<itemid_t> >(goldResult));

    std::list<itemid_t> resultList;
    orderManager.getFreqItemSets(goldResult.size(), inputItems, resultList);

    cout << "given input items (";
    copy(inputItems.begin(), inputItems.end(), COUT_ITER);
    cout << "), get other items in one order (";
    copy(resultList.begin(), resultList.end(), COUT_ITER);
    cout << "), expected items (";
    copy(goldResult.begin(), goldResult.end(), COUT_ITER);
    cout << ")";
    cout << endl;

    // sort gold items
    sort(goldResult.begin(), goldResult.end());

    // check recommend items
    vector<itemid_t> resultVec(resultList.begin(), resultList.end());
    sort(resultVec.begin(), resultVec.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(resultVec.begin(), resultVec.end(),
                                  goldResult.begin(), goldResult.end());
}

}


BOOST_AUTO_TEST_SUITE(OrderManagerTest)

BOOST_AUTO_TEST_CASE(checkOrder)
{
    bfs::remove_all(ORDER_HOME_STR);
    bfs::create_directories(ORDER_HOME_STR);

    // add items first
    bfs::path itemPath(bfs::path(ORDER_HOME_STR) / ITEM_DB_STR);
    bfs::path maxIdPath(bfs::path(ORDER_HOME_STR) / MAX_ID_STR);
    ItemManager itemManager(itemPath.string(), maxIdPath.string());
    const itemid_t MAX_ITEM_ID = 50;
    for (itemid_t i = 1; i <= MAX_ITEM_ID; ++i)
    {
        BOOST_CHECK(itemManager.addItem(i, Item()));
    }

    OrderManager orderManager(ORDER_HOME_STR, &itemManager);
    orderManager.setMinThreshold(1);
    {
    itemid_t myints[] = {1,2,3,4};
    std::list<itemid_t> transaction (myints, myints + sizeof(myints) / sizeof(itemid_t) );
    orderManager.addOrder(transaction);
    }

    {
    itemid_t myints[] = {2,3};
    std::list<itemid_t> transaction (myints, myints + sizeof(myints) / sizeof(itemid_t) );
    orderManager.addOrder(transaction);
    }

    {
    itemid_t myints[] = {3,4,5};
    std::list<itemid_t> transaction (myints, myints + sizeof(myints) / sizeof(itemid_t) );
    orderManager.addOrder(transaction);
    }

    // no need to flush as the items and orders could be fetched in realtime
    //orderManager.flush();

    orderManager.buildFreqItemsets();
    FrequentItemSetResultType results;
    orderManager.getAllFreqItemSets(100, 2, results);
    for(FrequentItemSetResultType::iterator resultIt = results.begin(); resultIt != results.end(); ++resultIt)
    {
        std::vector<itemid_t>& items = resultIt->first;
        cout<<"ItemSets: freq: "<<resultIt->second<<" items: ";
        copy(items.begin(), items.end(), COUT_ITER);
        cout<<endl;
    }

    checkGetFreqItemSets(orderManager, "1 2 3", "4");
    checkGetFreqItemSets(orderManager, "1 2", "3 4");
    checkGetFreqItemSets(orderManager, "2 3 4", "1");
    checkGetFreqItemSets(orderManager, "3 4", "1 2 5");
    checkGetFreqItemSets(orderManager, "4 5", "3");
    checkGetFreqItemSets(orderManager, "1", "2 3 4");
    checkGetFreqItemSets(orderManager, "2", "1 3 4");
    checkGetFreqItemSets(orderManager, "3", "1 2 4 5");
    checkGetFreqItemSets(orderManager, "4", "1 2 3 5");
    checkGetFreqItemSets(orderManager, "5", "3 4");
}

BOOST_AUTO_TEST_SUITE_END() 

