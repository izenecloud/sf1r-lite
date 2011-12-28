/**
 * @author Yingfeng Zhang
 * @date 2010-08-13
 */

#include <boost/test/unit_test.hpp>

#include <ir/index_manager/index/MockIndexReader.h>
#include <search-manager/ANDDocumentIterator.h>

using namespace sf1r;
using namespace izenelib::ir::indexmanager;

BOOST_AUTO_TEST_SUITE( ANDDocumentIterator_Suite )

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

    void doc_item(RankDocumentProperty& rankDocumentProperty){}

    void df_ctf(DocumentFrequencyInProperties& dfmap, CollectionTermFrequencyInProperties& ctfmap){}

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

BOOST_AUTO_TEST_CASE(and_test)
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

        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
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

        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        ANDDocumentIterator iter;
        MockTermDocumentIterator* pTermDocIterator1 = new MockTermDocumentIterator(1, 1, &indexer,"title",1);
        iter.add(pTermDocIterator1);
        MockTermDocumentIterator* pTermDocIterator2 = new MockTermDocumentIterator(1, 1, &indexer,"title",1);
        pTermDocIterator2->setNot(true);
        iter.add(pTermDocIterator2);

        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        ANDDocumentIterator iter;
        MockTermDocumentIterator* pTermDocIterator1 = new MockTermDocumentIterator(3, 1, &indexer,"title",1);
        iter.add(pTermDocIterator1);
        MockTermDocumentIterator* pTermDocIterator2 = new MockTermDocumentIterator(16, 1, &indexer,"title",1);
        pTermDocIterator2->setNot(true);
        iter.add(pTermDocIterator2);

        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 2U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

}

BOOST_AUTO_TEST_SUITE_END()
