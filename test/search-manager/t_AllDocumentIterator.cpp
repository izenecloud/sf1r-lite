#include <boost/test/unit_test.hpp>

#include <index-manager/IndexManager.h>
#include <search-manager/AllDocumentIterator.h>

using namespace sf1r;
using namespace izenelib::ir::indexmanager;

BOOST_AUTO_TEST_SUITE( AllDocumentIterator_Suite )

static unsigned maxDoc = 100;

BOOST_AUTO_TEST_CASE(alldociter_test)
{
    {
        boost::shared_ptr<IndexManager::FilterBitmapT> pFilterIdSet(new IndexManager::FilterBitmapT);

        pFilterIdSet->set(1);
        pFilterIdSet->set(2);
        pFilterIdSet->set(3);
        pFilterIdSet->set(4);
        pFilterIdSet->set(5);
        pFilterIdSet->set(6);
        pFilterIdSet->set(10);
        pFilterIdSet->set(20);
        pFilterIdSet->set(30);
        pFilterIdSet->set(33);
        pFilterIdSet->set(40);
        pFilterIdSet->set(50);
        pFilterIdSet->set(100);

        IndexManager::FilterTermDocFreqsT* pFilterTermDocFreqs = new IndexManager::FilterTermDocFreqsT(pFilterIdSet);
        AllDocumentIterator* pIterator = new AllDocumentIterator(pFilterTermDocFreqs, maxDoc);

        //pIterator->next();
        unsigned doc = pIterator->skipTo(2);
        BOOST_CHECK_EQUAL(doc, 7U);
        doc = pIterator->skipTo(3);
        BOOST_CHECK_EQUAL(doc, 7U);
        doc = pIterator->skipTo(4);
        BOOST_CHECK_EQUAL(doc, 7U);
        doc = pIterator->skipTo(5);
        BOOST_CHECK_EQUAL(doc, 7);
        doc = pIterator->skipTo(6);
        BOOST_CHECK_EQUAL(doc, 7U);
        doc = pIterator->skipTo(8);
        BOOST_CHECK_EQUAL(doc, 9U);
        doc = pIterator->skipTo(9);
        BOOST_CHECK_EQUAL(doc, 11U);
        doc = pIterator->skipTo(10);
        BOOST_CHECK_EQUAL(doc, 11U);
        doc = pIterator->skipTo(20);
        BOOST_CHECK_EQUAL(doc, 21U);
        doc = pIterator->skipTo(30);
        BOOST_CHECK_EQUAL(doc, 31U);
        doc = pIterator->skipTo(40);
        BOOST_CHECK_EQUAL(doc, 41U);
        doc = pIterator->skipTo(50);
        BOOST_CHECK_EQUAL(doc, 51U);
        doc = pIterator->skipTo(60);
        BOOST_CHECK_EQUAL(doc, 61U);
        doc = pIterator->skipTo(98);
        BOOST_CHECK_EQUAL(doc, 99U);
        doc = pIterator->skipTo(99);
        BOOST_CHECK_EQUAL(doc, -1U);
        doc = pIterator->skipTo(100);
        BOOST_CHECK_EQUAL(doc, -1U);
    }

    {
        boost::shared_ptr<IndexManager::FilterBitmapT> pFilterIdSet(new IndexManager::FilterBitmapT);

        pFilterIdSet->set(1);
        pFilterIdSet->set(2);
        pFilterIdSet->set(3);
        pFilterIdSet->set(4);
        pFilterIdSet->set(5);
        pFilterIdSet->set(6);
        pFilterIdSet->set(10);
        pFilterIdSet->set(20);
        pFilterIdSet->set(30);
        pFilterIdSet->set(33);
        pFilterIdSet->set(40);
        pFilterIdSet->set(50);
        pFilterIdSet->set(100);

        IndexManager::FilterTermDocFreqsT* pFilterTermDocFreqs = new IndexManager::FilterTermDocFreqsT(pFilterIdSet);
        AllDocumentIterator* pIterator = new AllDocumentIterator(pFilterTermDocFreqs, maxDoc);

        while(pIterator->next())
        {
            std::cout<<"doc "<<pIterator->doc()<<std::endl;
        }
    }

}

BOOST_AUTO_TEST_SUITE_END()
