/*
 * File:   t_ScdIndex.cpp
 * Author: Paolo D'Apice
 *
 * Created on September 11, 2012, 10:02 AM
 */

#include "ScdIndexUtils.hpp"
#include "Timer.hpp"
#include "common/ScdIndex.h"
#include <boost/scoped_ptr.hpp>
#include <boost/test/unit_test.hpp>

#define DOCID(in) sf1r::Utilities::md5ToUint128(boost::str(boost::format("%032d") % in));
#define TITLE(in) boost::str(boost::format("Title %d") % in)

namespace test {
SCD_INDEX_PROPERTY_TAG(Title);
}

void doTest(const scd::ScdIndex<test::Title>& index, const long* expected) {
    typedef scd::ScdIndex<test::Title> ScdIndex;
    const size_t numdoc = index.size();

    // query by DOCID: hit
    for (size_t i = 1; i <= numdoc; ++i) {
        const ScdIndex::DocidType id = DOCID(i);
        long offset;
        BOOST_REQUIRE(index.getOffset(id, offset));
        BOOST_CHECK_EQUAL(expected[i-1], offset);
    }

    // query by Title: hit
    // single value
    for (size_t i = 1; i <= numdoc; i += 2) {
        const ScdIndex::PropertyType title(TITLE(i));
        std::vector<offset_type> offsets;
        BOOST_REQUIRE(index.getOffsetList(title, offsets));
        BOOST_CHECK_EQUAL(1, offsets.size());
        BOOST_CHECK_EQUAL(expected[i-1], offsets[0]);
    }

    // multiple values
    {
        const ScdIndex::PropertyType title("Title T");
        std::vector<offset_type> offsets;
        BOOST_REQUIRE(index.getOffsetList(title, offsets));
        BOOST_CHECK_EQUAL(5, offsets.size());
        for (size_t i = 1, j = 0; i <= numdoc; i += 2, ++j) {
            BOOST_CHECK_EQUAL(expected[i], offsets[j]);
        }
    }

    // query by DOCID: miss
    {
        long offset;
        BOOST_CHECK(not index.getOffset(ScdIndex::DocidType(0), offset));
        BOOST_CHECK(not index.getOffset(ScdIndex::DocidType(11), offset));
    }

    // query by Title: miss
    {
        std::vector<offset_type> offsets;
        BOOST_CHECK(not index.getOffsetList(ScdIndex::PropertyType("Title 0"), offsets));
        BOOST_CHECK(not index.getOffsetList(ScdIndex::PropertyType("Title 11"), offsets));
    }
}

/* Test indexing and document retrieval. */
BOOST_AUTO_TEST_CASE(test_index) {
    typedef scd::ScdIndex<test::Title> ScdIndex;

    const size_t DOC_NUM = 10;
    fs::path path = test::createScd("scd-index/test_index.scd", DOC_NUM);
    fs::path dir1 = test::createTempDir("scd-index/docid", true);
    fs::path dir2 = test::createTempDir("scd-index/title", true);

    // expected values
    const long expected[] = { 0, 101, 195, 289, 383, 477, 571, 665, 759, 853 };

    // build index
    ScdIndex* index = ScdIndex::build(path.string(), dir1.string(), dir2.string());
    BOOST_REQUIRE(fs::exists(dir1) and fs::is_directory(dir1) and not fs::is_empty(dir1));
    BOOST_REQUIRE(fs::exists(dir2) and fs::is_directory(dir2) and not fs::is_empty(dir2));
    BOOST_CHECK_EQUAL(DOC_NUM, index->size());

    // perform test
    doTest(*index, expected);

    // load from file
    {
        boost::scoped_ptr<ScdIndex> loaded(ScdIndex::load(dir1.string(), dir2.string()));
        BOOST_CHECK_EQUAL(index->size(), loaded->size());
        // perform the same test
        doTest(*loaded, expected);

        // docid traversal
        for (ScdIndex::DocidIterator i = index->begin(), j = loaded->begin(), end = index->end();
                i != end; ++i, ++j) {
            BOOST_CHECK(i->first == j->first);
            BOOST_CHECK(i->second == j->second);
        }

        // property traversal
        for (ScdIndex::PropertyIterator i = index->pbegin(), j = loaded->pbegin(), end = index->pend();
                i != end; ++i, ++j) {
            BOOST_CHECK(i->first == j->first);
            const offset_list& oi = i->second;
            const offset_list& oj = j->second;
            BOOST_CHECK_EQUAL(oi.size(), oj.size());
            for (offset_list::const_iterator ii = oi.begin(), jj = oj.begin();
                    ii != oi.end(); ++ii, ++jj) {
                BOOST_CHECK_EQUAL(*ii, *jj);
            }
        }
    }

    // close db
    delete index;

    test::databaseSize(dir1, dir2);
}

/* enable performance test on a big SCD */
#define TEST_SCD_INDEX_PERFORMANCE 0

#if TEST_SCD_INDEX_PERFORMANCE
/* Index and serialize a _big_ SCD file. */
BOOST_AUTO_TEST_CASE(test_performance) {
    typedef scd::ScdIndex<> ScdIndex;
#if 1
    // create a sample SCD file as defined in the fixture.
    fs::path path = test::createScd("test_performance.scd", 21e5);
#else
    // load a real SCD file
    //fs::path path = "/home/paolo/tmp/B-00-201209051302-28047-I-C.SCD";
    fs::path path = "/home/paolo/tmp/B-00-201207282137-29781-U-C.SCD";
    BOOST_REQUIRE(fs::exists(path));
#endif
    fs::path dir1 = test::createTempDir("scd-index/docid", true);
    fs::path dir2 = test::createTempDir("scd-index/uuid", true);

    // measure elapsed time between a tic() and a toc()
    Timer timer;

    // build index
    {
        timer.tic();
        boost::scoped_ptr<ScdIndex> index(ScdIndex::build(path.string(), dir1.string(), dir2.string()));
        timer.toc();
        std::cout << "\nIndexing time:\n\t" << timer.seconds() << " seconds" << std::endl;
    }

    // load index
    {
        timer.tic();
        boost::scoped_ptr<ScdIndex> loaded(ScdIndex::load(dir1.string(), dir2.string()));
        timer.toc();
        std::cout << "Loading time:\n\t" << timer.seconds() << " seconds" << std::endl;

        timer.tic();
        for (ScdIndex::DocidIterator i = loaded->begin(), end = loaded->end(); i != end; ++i);
        timer.toc();
        std::cout << "Docid traversal time:\n\t" << timer.seconds() << " seconds" << std::endl;

        timer.tic();
        for (ScdIndex::PropertyIterator i = loaded->pbegin(), end = loaded->pend(); i != end; ++i);
        timer.toc();
        std::cout << "Property traversal time:\n\t" << timer.seconds() << " seconds" << std::endl;
    }

    // check size
    test::databaseSize(dir1, dir2);
}
#endif
