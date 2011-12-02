/**
 * @author Wei Cao
 * @date 2009-10-15
 */

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include <ir/index_manager/index/MockIndexReader.h>
#include <search-manager/PhraseDocumentIterator.h>

using namespace sf1r;
using namespace izenelib::ir::indexmanager;

BOOST_AUTO_TEST_SUITE( PhraseDocumentIterator_suite )

static std::vector<uint32_t> make_query(const std::string& query)
{
    std::vector<uint32_t> tl;
    uint32_t t;

    std::stringstream ss(query);
    while(!ss.eof()) {
        ss >> t;
        tl.push_back(t);
    }
    return tl;
}

BOOST_AUTO_TEST_CASE(simple)
{
    MockIndexReaderWriter indexer;
    indexer.insertDoc(0, "title", "1 2 3 4 5 6");
    indexer.insertDoc(1, "title", "1 2 3 4 5 8 9 10");
    indexer.insertDoc(2, "title", "5 6 7 8 9 10 11");
    indexer.insertDoc(3, "title", "9 10 10 10 13 14 15 16 10");
    vector<unsigned> termIndexes;

    {
        PhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query(""), termIndexes, 0U, &indexer, "title",1,false);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        PhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("5"), termIndexes, 0U, &indexer, "title",1,false);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 2U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        PhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("1 2"), termIndexes, 0U, &indexer, "title",1,false);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }
/*
    {
        PhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("8 9 10"), termIndexes, 0U, &indexer, "title",1,false);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 2U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        PhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("8 9 10 11"), termIndexes, 0U, &indexer, "title",1,false);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 2U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        PhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("8 9 10 11 12"), termIndexes, 0U, &indexer, "title",1,false);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        PhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("6 9 10 11"), termIndexes, 0U, &indexer, "title",1,false);
        BOOST_CHECK_EQUAL(iter.next(), true );
    }
*/
}

//BOOST_AUTO_TEST_CASE(longer)
void log()
{
    MockIndexReaderWriter indexer;
    std::string doc;
    for(int i = 0; i<100; i++) {
        doc = doc + boost::lexical_cast<std::string>(i) + " ";
    }
    for(int i = 0; i<100; i++) {
        indexer.insertDoc(i, "title", doc);
    }
    vector<unsigned> termIndexes;

    {
        PhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("1 2"), termIndexes, 0U, &indexer, "title",1,false);
        for(int i = 0; i< 100; i++) {
            BOOST_CHECK_EQUAL(iter.next(), true );
            BOOST_CHECK_EQUAL(iter.doc(), (unsigned int)i );
        }
        BOOST_CHECK_EQUAL(iter.next(), false );
    }
}

BOOST_AUTO_TEST_SUITE_END()
