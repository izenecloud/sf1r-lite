#include "OrderManagerTestFixture.h"
#include "test_util.h"
#include <recommend-manager/storage/OrderManager.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <vector>
#include <list>
#include <algorithm>

using namespace std;

namespace
{
using namespace sf1r;

const int64_t INDEX_MEMORY_SIZE = 10000000;
ostream_iterator<itemid_t> COUT_ITER(cout, ",");
}

namespace sf1r
{

void OrderManagerTestFixture::setTestDir(const std::string& dir)
{
    testDir_ = dir;
    boost::filesystem::remove_all(testDir_);
}

void OrderManagerTestFixture::resetInstance()
{
    // flush first
    orderManager_.reset();

    orderManager_.reset(new OrderManager(testDir_, INDEX_MEMORY_SIZE));
    orderManager_->setMinThreshold(1);
}

void OrderManagerTestFixture::addOrder(const char* inputStr)
{
    vector<itemid_t> items;
    split_str_to_items(inputStr, items);

    orderManager_->addOrder(items);
}

void OrderManagerTestFixture::checkFreqItems(const char* inputItemStr, const char* goldItemStr)
{
    std::list<itemid_t> inputItems;
    split_str_to_items(inputItemStr, inputItems);

    vector<itemid_t> goldResult;
    split_str_to_items(goldItemStr, goldResult);

    std::list<itemid_t> resultList;
    orderManager_->getFreqItemSets(goldResult.size(), inputItems, resultList);

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

void OrderManagerTestFixture::checkFreqBundles()
{
    orderManager_->buildFreqItemsets();

    FrequentItemSetResultType results;
    orderManager_->getAllFreqItemSets(100, 2, results);

    for (FrequentItemSetResultType::iterator resultIt = results.begin(); resultIt != results.end(); ++resultIt)
    {
        std::vector<itemid_t>& items = resultIt->first;
        cout<<"ItemSets: freq: "<<resultIt->second<<" items: ";
        copy(items.begin(), items.end(), COUT_ITER);
        cout<<endl;
    }
}

} // namespace sf1r
