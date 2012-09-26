/*
 * File:   t_ScdIndex.cpp
 * Author: Paolo D'Apice
 *
 * Created on September 11, 2012, 10:02 AM
 */

#include <boost/test/unit_test.hpp>

#include "ScdBuilder.h"
#include "Command.hpp"
#include "Timer.hpp"
#include "common/ScdIndex.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/scoped_ptr.hpp>

namespace fs = boost::filesystem;

/// Test fixture and helper methods.
struct TestFixture {
    fs::path tmpdir;
    TestFixture() {
        // TODO: sistemare questo che è un po' brutto
        tmpdir = createTempDir("scd-index");
        BOOST_REQUIRE(fs::exists(tmpdir));
    }
    /// Create a sample SCD file.
    fs::path createScd(const std::string& name,
            const unsigned size, const unsigned start = 1) const {
        std::cout << "Creating temporary SCD file with "
                  << size << " documents ..." << std::endl;
        fs::path file = tmpdir/name;
        fs::remove(file);
        BOOST_REQUIRE(not fs::exists(file));
        ScdBuilder scd(file);
        for (unsigned i = start; i < start + size; ++i) {
            scd("DOCID") << boost::format("%032d") % i;
            if (i % 2 == 0)
                scd("Title") << "Title T";
            else
                scd("Title") << "Title " << i;
            scd("uuid") << boost::format("%032d") % (i*i/2);
        }
        BOOST_REQUIRE(fs::exists(file));
        return file;
    }
    /// Create a temporary file.
    fs::path tempFile(const std::string& name) const {
        fs::path file = tmpdir/name;
        fs::remove(file);
        BOOST_REQUIRE(not fs::exists(file));
        return file;
    }
    /// Create a temporary directory.
    fs::path createTempDir(const std::string& name, const bool empty = false) const {
        fs::path dir = fs::temp_directory_path()/name;
        if (empty) fs::remove_all(dir);
        fs::create_directories(dir);
        return dir;
    }
};

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

void databaseSize(const fs::path& p1, const fs::path& p2) {
    size_t s1, s2;
    Command("du " + p1.string() + " | awk '{ print $1 }'").stream() >> s1;
    Command("du " + p2.string() + " | awk '{ print $1 }'").stream() >> s2;
    std::cout << p1.string() << ": " << s1 << " KiB\n"
              << p2.string() << ": " << s2 << " KiB\n"
              << "* total: " << s1 + s2 << " KiB" << std::endl;
}

/* Test indexing and document retrieval. */
BOOST_FIXTURE_TEST_CASE(test_index, TestFixture) {
    typedef scd::ScdIndex<test::Title> ScdIndex;

    const size_t DOC_NUM = 10;
    fs::path path = createScd("test_index.scd", DOC_NUM);
    fs::path dir1 = createTempDir("scd-index/docid", true);
    fs::path dir2 = createTempDir("scd-index/title", true);

    // expected values
    const long expected[] = { 0, 101, 195, 289, 383, 477, 571, 665, 759, 853 };

    // build index
    ScdIndex* indexptr = ScdIndex::build(path.string(), dir1.string(), dir2.string());
    BOOST_REQUIRE(fs::exists(dir1) and fs::is_directory(dir1) and not fs::is_empty(dir1));
    BOOST_REQUIRE(fs::exists(dir2) and fs::is_directory(dir2) and not fs::is_empty(dir2));
    BOOST_CHECK_EQUAL(DOC_NUM, indexptr->size());

    // perform test
    doTest(*indexptr, expected);

    // load from file
    {
        boost::scoped_ptr<ScdIndex> loaded(ScdIndex::load(dir1.string(), dir2.string()));
        BOOST_CHECK_EQUAL(indexptr->size(), loaded->size());
        // perform the same test
        doTest(*loaded, expected);
    }

    // close db
    delete indexptr;

    databaseSize(dir1, dir2);
}

/* enable performance test on a big SCD */
#define TEST_SCD_INDEX_PERFORMANCE 0

#if TEST_SCD_INDEX_PERFORMANCE
/* Index and serialize a _big_ SCD file. */
BOOST_FIXTURE_TEST_CASE(test_performance, TestFixture) {
    typedef scd::ScdIndex<> ScdIndex;
#if 0
    // create a sample SCD file as defined in the fixture.
    fs::path path = createScd("test_performance.scd", 21e5);
#else
    // load a real SCD file
    //fs::path path = "/home/paolo/tmp/B-00-201209051302-28047-I-C.SCD";
    fs::path path = "/home/paolo/tmp/B-00-201207282137-29781-U-C.SCD";
    BOOST_REQUIRE(fs::exists(path));
#endif
    fs::path dir1 = createTempDir("scd-index/docid", true);
    fs::path dir2 = createTempDir("scd-index/uuid", true);

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
    }
    databaseSize(dir2, dir2);
}
#endif
