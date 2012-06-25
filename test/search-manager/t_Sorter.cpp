/**
 * @author Yingfeng Zhang
 * @date 2010-08-23
 */

#include <boost/test/unit_test.hpp>

#include <search-manager/Sorter.h>
#include <search-manager/HitQueue.h>

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
    void loadPropertyDataForSorting(const string& property, uint64_t* &data, size_t& size)
    {
        data = new uint64_t[MAXDOC];
        size = MAXDOC;
        if(property == "date")
        {
            for(size_t i = 0; i < MAXDOC; ++i)
                data[i] = i;
        }
        else
        {
            for(size_t i = 0; i < MAXDOC; ++i)
                data[i] = MAXDOC - i;
        }
    }

};


BOOST_AUTO_TEST_SUITE( Sorter_suite )

BOOST_AUTO_TEST_CASE(sorter1)
{
    MockIndexManager indexManager;
    uint64_t* data;
    size_t size;
    indexManager.loadPropertyDataForSorting("date", data, size);

    boost::shared_ptr<PropertyData> propDate(new PropertyData(UNSIGNED_INT_PROPERTY_TYPE, data, size));
    SortPropertyComparator* pComparator =
        new SortPropertyComparator(propDate);

    boost::shared_ptr<Sorter> pSorter(new Sorter(NULL));
    SortProperty* pSortProperty = new SortProperty("date", UNSIGNED_INT_PROPERTY_TYPE, pComparator, SortProperty::AUTO, false);
    pSorter->addSortProperty(pSortProperty);
    PropertySortedHitQueue* scoreItemQueue = new PropertySortedHitQueue(pSorter, MAXDOC);
    //ScoreSortedHitQueue* scoreItemQueue = new ScoreSortedHitQueue(MAXDOC);
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
    uint64_t* data1;
    size_t size;
    indexManager.loadPropertyDataForSorting("date", data1, size);

    boost::shared_ptr<PropertyData> propDate1(new PropertyData(UNSIGNED_INT_PROPERTY_TYPE, data1, size));
    SortPropertyComparator* pComparator1 = 
        new SortPropertyComparator(propDate1);

    uint64_t* data2;
    indexManager.loadPropertyDataForSorting("count", data2, size);

    boost::shared_ptr<PropertyData> propDate2(new PropertyData(UNSIGNED_INT_PROPERTY_TYPE, data2, size));
    SortPropertyComparator* pComparator2 = 
        new SortPropertyComparator(propDate2);

    boost::shared_ptr<Sorter> pSorter(new Sorter(NULL));
    SortProperty* pSortProperty1 = new SortProperty("date", UNSIGNED_INT_PROPERTY_TYPE,pComparator1, SortProperty::AUTO, false);
    pSorter->addSortProperty(pSortProperty1);
    SortProperty* pSortProperty2 = new SortProperty("count", UNSIGNED_INT_PROPERTY_TYPE,pComparator2,SortProperty::AUTO, false);
    pSorter->addSortProperty(pSortProperty2);
    PropertySortedHitQueue* scoreItemQueue = new PropertySortedHitQueue(pSorter, MAXDOC);

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
    uint64_t* data1;
    size_t size;
    indexManager.loadPropertyDataForSorting("date", data1, size);

    boost::shared_ptr<PropertyData> propDate1(new PropertyData(UNSIGNED_INT_PROPERTY_TYPE, data1, size));
    SortPropertyComparator* pComparator1 =
        new SortPropertyComparator(propDate1);

    uint64_t* data2;
    indexManager.loadPropertyDataForSorting("count", data2, size);

    boost::shared_ptr<PropertyData> propDate2(new PropertyData(UNSIGNED_INT_PROPERTY_TYPE, data2, size));
    SortPropertyComparator* pComparator2 = 
        new SortPropertyComparator(propDate2);

    boost::shared_ptr<Sorter> pSorter(new Sorter(NULL));
    SortProperty* pSortProperty2 = new SortProperty("count", UNSIGNED_INT_PROPERTY_TYPE,pComparator2,SortProperty::AUTO, false);
    pSorter->addSortProperty(pSortProperty2);
    SortProperty* pSortProperty1 = new SortProperty("date", UNSIGNED_INT_PROPERTY_TYPE,pComparator1,SortProperty::AUTO, false);
    pSorter->addSortProperty(pSortProperty1);
    PropertySortedHitQueue* scoreItemQueue = new PropertySortedHitQueue(pSorter, MAXDOC);

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
