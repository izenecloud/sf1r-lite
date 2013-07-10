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

//#define USTRING(in) izenelib::util::UString((in), izenelib::util::UString::UTF_8)
#define USTRING(in) ScdPropertyValueType(in)
#define DOCID(in) sf1r::Utilities::md5ToUint128(test::getDocid(in))

#define PRINT(what) std::cout << what << std::endl

namespace test {
SCD_INDEX_PROPERTY_TAG(Title);
}

void checkDoc(const SCDDoc& doc, size_t i) {
    BOOST_CHECK_EQUAL("DOCID", doc[0].first);
    BOOST_CHECK_EQUAL(USTRING(test::getDocid(i)), doc[0].second);

    BOOST_CHECK_EQUAL("Title", doc[1].first);
    BOOST_CHECK_EQUAL(USTRING(test::getTitle(i)), doc[1].second);

    BOOST_CHECK_EQUAL("uuid", doc[2].first);
    BOOST_CHECK_EQUAL(USTRING(test::getUuid(i)), doc[2].second);
}

void doTest(scd::ScdIndex<test::Title>& index) {
    typedef scd::ScdIndex<test::Title> ScdIndex;
    const size_t numdoc = index.size();
    const ScdIndex::iterator end;

    // query by DOCID: hit
    for (size_t i = 1; i <= numdoc; ++i) {
        const ScdIndex::DocidType id = DOCID(i);

        ScdIndex::iterator it = index.findDoc(id);
        BOOST_CHECK(it != end);
        //PRINT(*it);
        checkDoc(*it, i);
        BOOST_CHECK(++it == end);
    }

    // query by Title: hit
    // single value
    for (size_t i = 1; i <= numdoc; i += 2) {
        const ScdIndex::PropertyType title(test::getTitle(i));

        ScdIndex::iterator it = index.find(title);
        BOOST_CHECK(it != end);
        //PRINT(*it);
        checkDoc(*it, i);
        BOOST_CHECK(++it == end);
    }

    // multiple values
    {
        const ScdIndex::PropertyType title("Title T");

        ScdIndex::iterator it = index.find(title);
        for (size_t j = 2; j < numdoc; j += 2, ++it) {
            BOOST_CHECK(it != end);
            //PRINT(*it);
            checkDoc(*it, j);
        }
        BOOST_CHECK(++it == end);
    }

    // query by DOCID: miss
    {
        BOOST_CHECK(index.findDoc(DOCID(0)) == end);
        BOOST_CHECK(index.findDoc(DOCID(11)) == end);
    }

    // query by Title: miss
    {
        BOOST_CHECK(index.find("Title 0") == end);
        BOOST_CHECK(index.find("Title 11") == end);
    }
}

/* Test indexing and document retrieval. */
BOOST_AUTO_TEST_CASE(test_index) {
    typedef scd::ScdIndex<test::Title> ScdIndex;

    const size_t DOC_NUM = 10;
    fs::path path = test::createScd("scd-index/test_index.scd", DOC_NUM);
    fs::path dir1 = test::createTempDir("scd-index/docid", true);
    fs::path dir2 = test::createTempDir("scd-index/title", true);

    // build index
    ScdIndex* index = ScdIndex::build(path.string(), dir1.string(), dir2.string());
    BOOST_REQUIRE(fs::exists(dir1) and fs::is_directory(dir1) and not fs::is_empty(dir1));
    BOOST_REQUIRE(fs::exists(dir2) and fs::is_directory(dir2) and not fs::is_empty(dir2));
    BOOST_CHECK_EQUAL(DOC_NUM, index->size());

    // perform test
    doTest(*index);
    size_t size = index->size();
    // close db
    delete index;

    // load from file
    {
        boost::scoped_ptr<ScdIndex> loaded(ScdIndex::load(path.string(), dir1.string(), dir2.string()));
        BOOST_CHECK_EQUAL(size, loaded->size());
        // perform the same test
        doTest(*loaded);
    }


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
        boost::scoped_ptr<ScdIndex> loaded(ScdIndex::load(path.string(), dir1.string(), dir2.string()));
        timer.toc();
        std::cout << "Loading time:\n\t" << timer.seconds() << " seconds" << std::endl;
    }

    // check size
    test::databaseSize(dir1, dir2);
}
#endif
