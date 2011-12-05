/**
 * @author Wei Cao
 * @date 2009-10-15
 */
#include "boost/tuple/tuple.hpp"
#include <boost/test/unit_test.hpp>

#include <ir/index_manager/index/MockIndexReader.h>
#include <search-manager/WildcardPhraseDocumentIterator.h>

using namespace sf1r;
using namespace izenelib::ir::indexmanager;

BOOST_AUTO_TEST_SUITE( WildcardPhraseDocumentIterator_suite )

static boost::tuple<std::vector<uint32_t>, std::vector<size_t>, std::vector<size_t> > make_query(const std::string& query)
{
    std::vector<uint32_t> tl;
    std::vector<size_t> asterisk_pos;
    std::vector<size_t> question_mark_pos;

    uint32_t t;
    char ch;
    std::stringstream ss(query);
    while(!ss.eof()) {
        ss >> ch;
        if( ch == '*') {
            asterisk_pos.push_back(tl.size());
        } else if( ch == '?' ) {
            question_mark_pos.push_back(tl.size());
        } else {
                ss.putback(ch);
                ss >> t;
                tl.push_back(t);
        }
    }

    return boost::make_tuple(tl, asterisk_pos, question_mark_pos);
}

BOOST_AUTO_TEST_CASE(simple)
{
    MockIndexReaderWriter indexer;
    indexer.insertDoc(0, "title", "1 2 3 4 5 6");
    indexer.insertDoc(1, "title", "1 2 3 4 5 8 9 10");
    indexer.insertDoc(2, "title", "5 6 7 8 9 10 11");
    indexer.insertDoc(3, "title", "9 10 10 10 13 14 15 16 10");
    indexer.insertDoc(4, "title", "3 5 8 7 9");
    indexer.insertDoc(5, "title", "3 5 8 10 9");
    vector<unsigned> termIndexes;

    {
        WildcardPhraseDocumentIterator_<MockIndexReaderWriter> iter( boost::get<0>(make_query("3 *")),
            termIndexes, boost::get<1>(make_query("3 *")), boost::get<2>(make_query("3 *")), 0U, &indexer, "title",1);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 4U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 5U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        WildcardPhraseDocumentIterator_<MockIndexReaderWriter> iter( boost::get<0>(make_query("* 3")),
            termIndexes, boost::get<1>(make_query("* 3")), boost::get<2>(make_query("* 3")), 0U, &indexer, "title",1);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 4U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 5U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        WildcardPhraseDocumentIterator_<MockIndexReaderWriter> iter( boost::get<0>(make_query("1 * 2")),
            termIndexes, boost::get<1>(make_query("1 * 2")), boost::get<2>(make_query("1 * 2")), 0U, &indexer, "title",1);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        WildcardPhraseDocumentIterator_<MockIndexReaderWriter> iter( boost::get<0>(make_query("8 * 9")),
            termIndexes, boost::get<1>(make_query("8 * 9")), boost::get<2>(make_query("8 * 9")), 0U, &indexer, "title",1);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 2U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 4U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 5U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        WildcardPhraseDocumentIterator_<MockIndexReaderWriter> iter( boost::get<0>(make_query("1 * 3")),
            termIndexes, boost::get<1>(make_query("1 * 3")), boost::get<2>(make_query("1 * 3")), 0U, &indexer, "title",1);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        WildcardPhraseDocumentIterator_<MockIndexReaderWriter> iter( boost::get<0>(make_query("1 * 4")),
            termIndexes, boost::get<1>(make_query("1 * 4")), boost::get<2>(make_query("1 * 4")), 0U, &indexer, "title",1);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        WildcardPhraseDocumentIterator_<MockIndexReaderWriter> iter( boost::get<0>(make_query("1 ? 3")),
            termIndexes, boost::get<1>(make_query("1 ? 3")), boost::get<2>(make_query("1 ? 3")), 0U, &indexer, "title",1);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 1U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        WildcardPhraseDocumentIterator_<MockIndexReaderWriter> iter( boost::get<0>(make_query("1 ? 4")),
            termIndexes, boost::get<1>(make_query("1 ? 4")), boost::get<2>(make_query("1 ? 4")), 0U, &indexer, "title",1);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }


    {
        WildcardPhraseDocumentIterator_<MockIndexReaderWriter> iter( boost::get<0>(make_query("1 2 3 * 6")),
            termIndexes, boost::get<1>(make_query("1 2 3 * 6")), boost::get<2>(make_query("1 2 3 * 6")), 0U, &indexer, "title",1);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 0U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        WildcardPhraseDocumentIterator_<MockIndexReaderWriter> iter( boost::get<0>(make_query("1 3 * 6")),
            termIndexes, boost::get<1>(make_query("1 3 * 6")), boost::get<2>(make_query("1 3 * 6")), 0U, &indexer, "title",1);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        WildcardPhraseDocumentIterator_<MockIndexReaderWriter> iter( boost::get<0>(make_query("1 3")),
            termIndexes, boost::get<1>(make_query("1 3")), boost::get<2>(make_query("1 3")), 0U, &indexer, "title",1);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }


    {
        WildcardPhraseDocumentIterator_<MockIndexReaderWriter> iter( boost::get<0>(make_query("5 * 9 ? 11")),
            termIndexes, boost::get<1>(make_query("5 * 9 ? 11")), boost::get<2>(make_query("5 * 9 ? 11")), 0U, &indexer, "title",1);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 2U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

    {
        WildcardPhraseDocumentIterator_<MockIndexReaderWriter> iter( boost::get<0>(make_query("9 * 13 ? 15 * 10")),
            termIndexes, boost::get<1>(make_query("9 * 13 ? 15 * 10")), boost::get<2>(make_query("9 * 13 ? 15 * 10")), 0U, &indexer, "title",1);
        BOOST_CHECK_EQUAL(iter.next(), true );
        BOOST_CHECK_EQUAL(iter.doc(), 3U);
        BOOST_CHECK_EQUAL(iter.next(), false );
    }

}


BOOST_AUTO_TEST_SUITE_END()
