/**
 * \file t_FilterDocumentIterator.cpp
 * \brief unit test for FilterDocumentIterator class
 * \date Nov 23, 2011
 * \author Xin Liu
 */

#include <boost/test/unit_test.hpp>

#include <ir/index_manager/index/MockIndexReader.h>
#include <index-manager/InvertedIndexManager.h>
#include <search-manager/ANDDocumentIterator.h>
#include <search-manager/FilterDocumentIterator.h>

using namespace sf1r;
using namespace izenelib::ir::indexmanager;

BOOST_AUTO_TEST_SUITE( FilterDocumentIterator_Suite )

static uint32_t *int_data;
static uint32_t * copy_data;
void init_data(int data_size, int& copy_length)
{
    std::cout<<"generating data... "<<std::endl;
    int MAXID = 1000;// 20000  100000000

    int_data = new unsigned[data_size];
    srand( (unsigned)time(NULL) );

    for (int i = 0; i < data_size; ++i) {
        int_data[i] = rand() % MAXID;
    }
    std::sort(int_data, int_data+ data_size);
    uint32_t* it = std::unique(int_data, int_data+ data_size);

    copy_length = it - int_data;
    cout<<"real_size = "<<copy_length<<endl;
    copy_data = new unsigned[copy_length];
    std::memcpy(copy_data, int_data, copy_length * sizeof(uint32_t));

    std::cout<<"done!\n";
}

///DocumentIterator for one term in one property
class MockTermDocumentIterator: public DocumentIterator
{
public:
    MockTermDocumentIterator(
            unsigned int termid,
            unsigned int colID,
            izenelib::ir::indexmanager::MockIndexReaderWriter* pIndexReader,
            const std::string& property,
            unsigned int propertyId
    )
        : termId_(termid)
        , colID_(colID)
        , property_(property)
        , propertyId_(propertyId)
        , pIndexReader_(pIndexReader)
        , pTermReader_(0)
        , pTermDocReader_(0)
    {
        accept();
    }

    ~MockTermDocumentIterator()
    {
        if(pTermReader_)
            delete pTermReader_;
        if(pTermDocReader_)
            delete pTermDocReader_;
    }

public:
    void add(DocumentIterator* pDocIterator){}

    void accept()
    {
        Term term(property_.c_str(),termId_);

        pTermReader_ = (MockTermReader*)pIndexReader_->getTermReader(colID_);
        pTermReader_->seek(&term);
        pTermDocReader_ = (MockTermDocFreqs*)pTermReader_->termPositions();
    }

    bool next()
    {
        if (pTermDocReader_)
            return pTermDocReader_->next();
        return false;
    }

    unsigned int doc()
    {
        BOOST_ASSERT(pTermDocReader_);
        return pTermDocReader_->doc();
    }

    unsigned int skipTo(unsigned int target)
    {
        return pTermDocReader_->skipTo(target);
    }

    void doc_item(RankDocumentProperty& rankDocumentProperty, unsigned propIndex = 0){}

    void df_cmtf(DocumentFrequencyInProperties& dfmap,
            CollectionTermFrequencyInProperties& ctfmap,
            MaxTermFrequencyInProperties& maxtfmap)
    {}

    izenelib::ir::indexmanager::count_t tf() { return MAX_COUNT; }

private:
    unsigned int termId_;

    unsigned int colID_;

    std::string property_;

    unsigned int propertyId_;

    izenelib::ir::indexmanager::MockIndexReaderWriter* pIndexReader_;

    izenelib::ir::indexmanager::MockTermReader* pTermReader_;

    izenelib::ir::indexmanager::MockTermDocFreqs* pTermDocReader_;

    unsigned int currDoc_;

};

BOOST_AUTO_TEST_CASE(filter_test)
{
    MockIndexReaderWriter indexer;
    indexer.insertDoc(0, "title", "1 2 3 4 5 6");
    indexer.insertDoc(1, "title", "1 2 3 4 5 8 9 10");
    indexer.insertDoc(2, "title", "3 5 6 7 8 9 10 11");
    indexer.insertDoc(3, "title", "3 9 10 10 10 13 14 15 16 10");
    {
        ANDDocumentIterator iter;
        MockTermDocumentIterator* pTermDocIterator1 = new MockTermDocumentIterator(1, 1, &indexer,"title",1);
        iter.add(pTermDocIterator1);
        MockTermDocumentIterator* pTermDocIterator2 = new MockTermDocumentIterator(2, 1, &indexer,"title",1);
        iter.add(pTermDocIterator2);

        boost::shared_ptr<InvertedIndexManager::FilterBitmapT> pFilterIdSet(new InvertedIndexManager::FilterBitmapT);
        pFilterIdSet->set(0);
        pFilterIdSet->set(2);
        InvertedIndexManager::FilterTermDocFreqsT* pFilterTermDocFreqs = new InvertedIndexManager::FilterTermDocFreqsT(pFilterIdSet);
        FilterDocumentIterator* pFilterIterator = new FilterDocumentIterator( pFilterTermDocFreqs );
        iter.add(pFilterIterator);

        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        ANDDocumentIterator iter;
        MockTermDocumentIterator* pTermDocIterator1 = new MockTermDocumentIterator(1, 1, &indexer,"title",1);
        iter.add(pTermDocIterator1);
        MockTermDocumentIterator* pTermDocIterator2 = new MockTermDocumentIterator(3, 1, &indexer,"title",1);
        iter.add(pTermDocIterator2);
        MockTermDocumentIterator* pTermDocIterator3 = new MockTermDocumentIterator(5, 1, &indexer,"title",1);
        iter.add(pTermDocIterator3);

        boost::shared_ptr<InvertedIndexManager::FilterBitmapT> pFilterIdSet(new InvertedIndexManager::FilterBitmapT);
        pFilterIdSet->set(2);
        pFilterIdSet->set(3);
        InvertedIndexManager::FilterTermDocFreqsT* pFilterTermDocFreqs = new InvertedIndexManager::FilterTermDocFreqsT(pFilterIdSet);
        FilterDocumentIterator* pFilterIterator = new FilterDocumentIterator( pFilterTermDocFreqs );
        iter.add(pFilterIterator);

        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        ANDDocumentIterator iter;
        MockTermDocumentIterator* pTermDocIterator1 = new MockTermDocumentIterator(1, 1, &indexer,"title",1);
        iter.add(pTermDocIterator1);
        MockTermDocumentIterator* pTermDocIterator2 = new MockTermDocumentIterator(1, 1, &indexer,"title",1);
        pTermDocIterator2->setNot(true);
        iter.add(pTermDocIterator2);

        boost::shared_ptr<InvertedIndexManager::FilterBitmapT> pFilterIdSet(new InvertedIndexManager::FilterBitmapT);
        pFilterIdSet->set(0);
        pFilterIdSet->set(1);
        InvertedIndexManager::FilterTermDocFreqsT* pFilterTermDocFreqs = new InvertedIndexManager::FilterTermDocFreqsT(pFilterIdSet);
        FilterDocumentIterator* pFilterIterator = new FilterDocumentIterator( pFilterTermDocFreqs );
        iter.add(pFilterIterator);

        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        ANDDocumentIterator iter;
        MockTermDocumentIterator* pTermDocIterator1 = new MockTermDocumentIterator(3, 1, &indexer,"title",1);
        iter.add(pTermDocIterator1);
        MockTermDocumentIterator* pTermDocIterator2 = new MockTermDocumentIterator(16, 1, &indexer,"title",1);
        pTermDocIterator2->setNot(true);
        iter.add(pTermDocIterator2);

        boost::shared_ptr<InvertedIndexManager::FilterBitmapT> pFilterIdSet(new InvertedIndexManager::FilterBitmapT);
        pFilterIdSet->set(0);
        pFilterIdSet->set(1);
        pFilterIdSet->set(2);
        pFilterIdSet->set(3);
        InvertedIndexManager::FilterTermDocFreqsT* pFilterTermDocFreqs = new InvertedIndexManager::FilterTermDocFreqsT(pFilterIdSet);
        FilterDocumentIterator* pFilterIterator = new FilterDocumentIterator( pFilterTermDocFreqs );
        iter.add(pFilterIterator);

        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 2U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        ANDDocumentIterator iter;
        boost::shared_ptr<InvertedIndexManager::FilterBitmapT> pFilterIdSet(new InvertedIndexManager::FilterBitmapT);

        int data_size = 100;
        int real_size = 0;
        init_data(data_size, real_size);

        int i = 0;

        for(; i < real_size; ++i)
        {
            pFilterIdSet->set(copy_data[i]);
        }
        // pFilterIdSet->set(0);
        // pFilterIdSet->set(2);
        // pFilterIdSet->set(3);

        InvertedIndexManager::FilterTermDocFreqsT* pFilterTermDocFreqs = new InvertedIndexManager::FilterTermDocFreqsT(pFilterIdSet);
        FilterDocumentIterator* pFilterIterator = new FilterDocumentIterator( pFilterTermDocFreqs );
        iter.add(pFilterIterator);

        clock_t start,finish;
        double totaltime;
        start = clock();

        finish = clock();
        totaltime=(double)(finish-start)/CLOCKS_PER_SEC;
        cout<<"\ntime = "<<totaltime<<"seconds！"<<endl;

        delete[] int_data;
        delete[] copy_data;

        // BOOST_CHECK_EQUAL(iter.next(), true );
        // BOOST_CHECK_EQUAL(iter.doc(), 0U);
        // BOOST_CHECK_EQUAL(iter.next(), true );
        // BOOST_CHECK_EQUAL(iter.doc(), 2U);
        // BOOST_CHECK_EQUAL(iter.next(), true );
        // BOOST_CHECK_EQUAL(iter.doc(), 3U);
        // BOOST_CHECK_EQUAL(iter.next(), false );
    }

}

BOOST_AUTO_TEST_SUITE_END()
