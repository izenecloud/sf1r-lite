/**
 * @author Wei Cao
 * @date 2009-10-15
 */

#include <boost/test/unit_test.hpp>

#include <ir/index_manager/index/MockIndexReader.h>

using namespace izenelib::ir::indexmanager;

BOOST_AUTO_TEST_SUITE( MockIndex_suite )


BOOST_AUTO_TEST_CASE(MockIndex)
{
    MockIndexReaderWriter indexer;
    indexer.insertDoc(0, "title", "1 2 3 4 5 6");
    indexer.insertDoc(1, "title", "1 2 3 4 5 8 9 10");
    indexer.insertDoc(2, "title", "5 6 7 8 9 10 11");
    indexer.insertDoc(3, "title", "9 10 10 10 13 14 15 16 10");

    BOOST_CHECK_EQUAL(indexer.numDocs(), 4U);
    BOOST_CHECK_EQUAL(indexer.maxDoc(), 3U);
    BOOST_CHECK_EQUAL(indexer.docFreq(0, &Term("title", 1)), 2U);
    BOOST_CHECK_EQUAL(indexer.docFreq(0, &Term("title", 6)), 2U);
    BOOST_CHECK_EQUAL(indexer.docFreq(0, &Term("title", 8)), 2U);
    BOOST_CHECK_EQUAL(indexer.docFreq(0, &Term("title", 10)), 3U);

    TermReader* reader = indexer.getTermReader(0);
    reader->seek(&Term("title", 10U));

    // Check termdocfreqs
    TermDocFreqs* freqs = reader->termDocFreqs();
    BOOST_CHECK_EQUAL(freqs->docFreq(), 3U);
    BOOST_CHECK_EQUAL(freqs->getCTF(), 6U);

    BOOST_CHECK_EQUAL(freqs->next(), true);
    BOOST_CHECK_EQUAL(freqs->doc(), 1U);
    BOOST_CHECK_EQUAL(freqs->freq(), 1U);

    BOOST_CHECK_EQUAL(freqs->next(), true);
    BOOST_CHECK_EQUAL(freqs->doc(), 2U);
    BOOST_CHECK_EQUAL(freqs->freq(), 1U);

    BOOST_CHECK_EQUAL(freqs->next(), true);
    BOOST_CHECK_EQUAL(freqs->doc(), 3U);
    BOOST_CHECK_EQUAL(freqs->freq(), 4U);

    BOOST_CHECK_EQUAL(freqs->next(), false);

    // Check termpostions
    TermPositions* posts = reader->termPositions();
    BOOST_CHECK_EQUAL(posts->next(), true);
    BOOST_CHECK_EQUAL(posts->nextPosition(), 7U);
    BOOST_CHECK_EQUAL(posts->nextPosition(), BAD_POSITION);

    BOOST_CHECK_EQUAL(posts->next(), true);
    BOOST_CHECK_EQUAL(posts->nextPosition(), 5U);
    BOOST_CHECK_EQUAL(posts->nextPosition(), BAD_POSITION);

    BOOST_CHECK_EQUAL(posts->next(), true);
    BOOST_CHECK_EQUAL(posts->nextPosition(), 1U);
    BOOST_CHECK_EQUAL(posts->nextPosition(), 2U);
    BOOST_CHECK_EQUAL(posts->nextPosition(), 3U);
    BOOST_CHECK_EQUAL(posts->nextPosition(), 8U);
    BOOST_CHECK_EQUAL(posts->nextPosition(), BAD_POSITION);

    BOOST_CHECK_EQUAL(posts->next(), false);
    delete reader;
}

BOOST_AUTO_TEST_SUITE_END()
