/**
 * @author Wei Cao
 * @date 2009-10-15
 */

#include <boost/test/unit_test.hpp>

#include <ir/index_manager/index/MockIndexReader.h>
#include <search-manager/NearbyPhraseDocumentIterator.h>

using namespace sf1r;
using namespace izenelib::ir::indexmanager;

BOOST_AUTO_TEST_SUITE( NearbyPhraseDocumentIterator )

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
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("1 2"),termIndexes, 0U, &indexer, "title",1, 2);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("2 1"), termIndexes,0U, &indexer, "title", 1, 2);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("1 3"), termIndexes,0U, &indexer, "title", 1, 2);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("3 1"),termIndexes, 0U, &indexer, "title", 1, 2);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("1 4"), termIndexes,0U, &indexer, "title", 1, 2);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("4 1"),termIndexes, 0U, &indexer, "title", 1, 2);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("1 5"),termIndexes, 0U, &indexer, "title", 1, 2);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("5 1"),termIndexes, 0U, &indexer, "title", 1, 2);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("1 2 4"),termIndexes, 0U, &indexer, "title", 1, 2);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("1 2 5"),termIndexes, 0U, &indexer, "title", 1, 2);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }


    {
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("2 1 4"), termIndexes,0U, &indexer, "title", 1, 2);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("2 4 1"),termIndexes, 0U, &indexer, "title", 1, 2);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("8 9 10 11"),termIndexes, 0U, &indexer, "title", 1, 2);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 2U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        NearbyPhraseDocumentIterator_<MockIndexReaderWriter> iter( make_query("6 9 10 11"),termIndexes, 0U, &indexer, "title", 1, 2);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

}

BOOST_AUTO_TEST_SUITE_END()
