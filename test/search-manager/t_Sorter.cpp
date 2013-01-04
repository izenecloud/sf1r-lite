/**
 * @author Yingfeng Zhang
 * @date 2010-08-23
 */

#include <boost/test/unit_test.hpp>

#include <search-manager/Sorter.h>
#include <search-manager/HitQueue.h>
#include <common/NumericPropertyTable.h>
#include <common/PropSharedLockSet.h>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/ref.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/random.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace sf1r;


#define MAXDOC 10

class MockIndexManager
{
public:
    MockIndexManager(){}

    ~MockIndexManager(){}

public:
    void loadPropertyDataForSorting(const string& property, boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable)
    {
        if (!numericPropertyTable)
            numericPropertyTable.reset(new NumericPropertyTable<int64_t>(INT64_PROPERTY_TYPE));
        numericPropertyTable->resize(MAXDOC);
        int64_t* data = (int64_t*)numericPropertyTable->getValueList();
        if (property == "date")
        {
            for (size_t i = 0; i < MAXDOC; ++i)
                data[i] = i;
        }
        else
        {
            for (size_t i = 0; i < MAXDOC; ++i)
                data[i] = MAXDOC - i;
        }
    }

};


BOOST_AUTO_TEST_SUITE( Sorter_suite )

BOOST_AUTO_TEST_CASE(sorter1)
{
    MockIndexManager indexManager;
    boost::shared_ptr<NumericPropertyTableBase> numericPropertyTable;
    indexManager.loadPropertyDataForSorting("date", numericPropertyTable);

    SortPropertyComparator* pComparator =
        new SortPropertyComparator(numericPropertyTable);

    boost::shared_ptr<Sorter> pSorter(new Sorter(NULL));
    SortProperty* pSortProperty = new SortProperty("date", INT64_PROPERTY_TYPE, pComparator, SortProperty::AUTO, false);
    pSorter->addSortProperty(pSortProperty);

    PropSharedLockSet propSharedLockSet;
    PropertySortedHitQueue* scoreItemQueue =
        new PropertySortedHitQueue(pSorter, MAXDOC, propSharedLockSet);

    for(size_t i = 0; i < MAXDOC; i++)
    {
        ScoreDoc d(i, i);
        scoreItemQueue->insert(d);
    }

    for(size_t i = 0; i<MAXDOC; i++) {
        ScoreDoc d = scoreItemQueue->pop();
        BOOST_CHECK_EQUAL(d.docId, i);
        //scoreItemQueue->pop();
    }

    delete scoreItemQueue;
}

BOOST_AUTO_TEST_CASE(sorter2)
{
    MockIndexManager indexManager;
    boost::shared_ptr<NumericPropertyTableBase> numericPropertyTable1;
    indexManager.loadPropertyDataForSorting("date", numericPropertyTable1);

    SortPropertyComparator* pComparator1 =
        new SortPropertyComparator(numericPropertyTable1);

    boost::shared_ptr<NumericPropertyTableBase> numericPropertyTable2;
    indexManager.loadPropertyDataForSorting("count", numericPropertyTable2);

    SortPropertyComparator* pComparator2 =
        new SortPropertyComparator(numericPropertyTable2);

    boost::shared_ptr<Sorter> pSorter(new Sorter(NULL));
    SortProperty* pSortProperty1 = new SortProperty("date", INT64_PROPERTY_TYPE,pComparator1, SortProperty::AUTO, false);
    pSorter->addSortProperty(pSortProperty1);
    SortProperty* pSortProperty2 = new SortProperty("count", INT64_PROPERTY_TYPE,pComparator2,SortProperty::AUTO, false);
    pSorter->addSortProperty(pSortProperty2);

    PropSharedLockSet propSharedLockSet;
    PropertySortedHitQueue* scoreItemQueue =
        new PropertySortedHitQueue(pSorter, MAXDOC, propSharedLockSet);

    for(size_t i = 0; i < MAXDOC; i++)
    {
        ScoreDoc d(i, i);
        scoreItemQueue->insert(d);
    }

    for(size_t i = 0; i<MAXDOC; i++) {
        ScoreDoc d = scoreItemQueue->top();
        BOOST_CHECK_EQUAL(d.docId, i);
        scoreItemQueue->pop();
    }

    delete scoreItemQueue;
}

BOOST_AUTO_TEST_CASE(sorter3)
{
    MockIndexManager indexManager;
    boost::shared_ptr<NumericPropertyTableBase> numericPropertyTable1;
    indexManager.loadPropertyDataForSorting("date", numericPropertyTable1);

    SortPropertyComparator* pComparator1 =
        new SortPropertyComparator(numericPropertyTable1);

    boost::shared_ptr<NumericPropertyTableBase> numericPropertyTable2;
    indexManager.loadPropertyDataForSorting("count", numericPropertyTable2);

    SortPropertyComparator* pComparator2 =
        new SortPropertyComparator(numericPropertyTable2);

    boost::shared_ptr<Sorter> pSorter(new Sorter(NULL));
    SortProperty* pSortProperty2 = new SortProperty("count", INT64_PROPERTY_TYPE,pComparator2,SortProperty::AUTO, false);
    pSorter->addSortProperty(pSortProperty2);
    SortProperty* pSortProperty1 = new SortProperty("date", INT64_PROPERTY_TYPE, pComparator1, SortProperty::AUTO, false);
    pSorter->addSortProperty(pSortProperty1);

    PropSharedLockSet propSharedLockSet;
    PropertySortedHitQueue* scoreItemQueue =
        new PropertySortedHitQueue(pSorter, MAXDOC, propSharedLockSet);

    for(size_t i = 0; i < MAXDOC; i++)
    {
        ScoreDoc d(i, i);
        scoreItemQueue->insert(d);
    }

    for(size_t i = 0; i<MAXDOC; i++) {
        ScoreDoc d = scoreItemQueue->top();
        BOOST_CHECK_EQUAL(d.docId, MAXDOC-1-i);
        scoreItemQueue->pop();
    }

    delete scoreItemQueue;
}

BOOST_AUTO_TEST_SUITE_END()
