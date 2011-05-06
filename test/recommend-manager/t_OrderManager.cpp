/// @file t_OrderManager.cpp
/// @brief test OrderManager

#include <util/ustring/UString.h>
#include <recommend-manager/OrderManager.h>
#include <common/JobScheduler.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

using namespace std;
using namespace boost;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
const char* ORDER_HOME_STR = "recommend_test/t_OrderManager";
}


BOOST_AUTO_TEST_SUITE(OrderManagerTest)

BOOST_AUTO_TEST_CASE(checkOrder)
{
    bfs::remove_all(ORDER_HOME_STR);
    bfs::create_directories(ORDER_HOME_STR);

    OrderManager orderManager(ORDER_HOME_STR);
    orderManager.setMinThreshold(1);
    orderid_t orderId = 0;
    {
    itemid_t myints[] = {1,2,3,4};
    std::list<itemid_t> transaction (myints, myints + sizeof(myints) / sizeof(itemid_t) );
    orderManager.addOrder(++orderId, transaction);
    }

    {
    itemid_t myints[] = {2,3};
    std::list<itemid_t> transaction (myints, myints + sizeof(myints) / sizeof(itemid_t) );
    orderManager.addOrder(++orderId, transaction);
    }

    {
    itemid_t myints[] = {3,4,5};
    std::list<itemid_t> transaction (myints, myints + sizeof(myints) / sizeof(itemid_t) );
    orderManager.addOrder(++orderId, transaction);
    }

    orderManager.flush();
    orderManager.buildFreqItemsets();
    FrequentItemSetResultType& results = orderManager.getAllFreqItemSets();
    for(FrequentItemSetResultType::iterator resultIt = results.begin(); resultIt != results.end(); ++resultIt)
    {
        std::vector<itemid_t>& items = resultIt->first;
        cout<<"ItemSets: freq: "<<resultIt->second<<" items: ";
        for(std::vector<itemid_t>::iterator itemIt = items.begin(); itemIt != items.end(); ++itemIt)
        {
            cout<<*itemIt<<",";
        }
        cout<<endl;
    }

    {
    itemid_t myints[] = {1,2};
    std::list<itemid_t> inputItems (myints, myints + sizeof(myints) / sizeof(itemid_t) );
    std::list<itemid_t> results;
    orderManager.getFreqItemSets(inputItems, results);
    cout<<"ItemSet:";
    for(std::list<itemid_t>::iterator it = results.begin(); it != results.end(); ++it)
    {
        cout<<*it<<",";
    }
    cout<<endl;
    }
}

BOOST_AUTO_TEST_SUITE_END() 

