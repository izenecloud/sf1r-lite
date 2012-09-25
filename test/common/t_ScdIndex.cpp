/*
 * File:   t_ScdIndex.cpp
 * Author: Paolo D'Apice
 *
 * Created on September 11, 2012, 10:02 AM
 */

#include <boost/test/unit_test.hpp>

#include "ScdBuilder.h"
#include "Timer.hpp"
#include "common/ScdIndex.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/scoped_ptr.hpp>

namespace fs = boost::filesystem;

/* enable print to stdout */
#define DEBUG_OUTPUT 1

/// Test fixture and helper methods.
struct TestFixture {
    fs::path tmpdir;
    TestFixture() {
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

#define DOCID(in) boost::str(boost::format("%032d") % in)
#define TITLE(in) boost::str(boost::format("Title %d") % in)

namespace test {
SCD_INDEX_PROPERTY_TAG(Title);
}

/* Test indexing and document retrieval. */
BOOST_FIXTURE_TEST_CASE(test_index, TestFixture) {
    typedef scd::ScdIndex<test::Title> ScdIndex;

    const unsigned DOC_NUM = 10;
    fs::path path = createScd("test_index.scd", DOC_NUM);
    fs::path dir1 = createTempDir("scd-index/docid", true);
    fs::path dir2 = createTempDir("scd-index/title", true);

    // build index
    boost::scoped_ptr<ScdIndex> indexptr(ScdIndex::build(path.string(), dir1.string(), dir2.string()));

    // using const reference
    const ScdIndex& index = *indexptr;
#if DEBUG_OUTPUT
    indexptr->dump();
#endif

    // expected values
    const long expected[] = { 0, 101, 195, 289, 383, 477, 571, 665, 759, 853 };

    // query by DOCID: hit
    for (size_t i = 1; i <= DOC_NUM; ++i) {
        const std::string id = DOCID(i);
        long offset;
        BOOST_REQUIRE(index.find(id, &offset));
        BOOST_CHECK_EQUAL(expected[i-1], offset);
    }
    // query by Title: hit
    // single value
    for (size_t i = 1; i <= DOC_NUM; i += 2) {
        const std::string title = TITLE(i);
        std::vector<offset_type> offsets;
        BOOST_REQUIRE(index.find(title, offsets));
        BOOST_CHECK_EQUAL(1, offsets.size());
        BOOST_CHECK_EQUAL(expected[i-1], offsets[0]);
    }
    // multiple values
    {
        const std::string title("Title T");
        std::vector<offset_type> offsets;
        BOOST_REQUIRE(index.find(title, offsets));
        BOOST_CHECK_EQUAL(5, offsets.size());
        for (size_t i = 1, j = 0; i <= DOC_NUM; i += 2, ++j) {
            BOOST_CHECK_EQUAL(expected[i], offsets[j]);
        }
    }

    // query by DOCID: miss
    {
        long offset;
        BOOST_CHECK(not index.find(DOCID(0), &offset));
        BOOST_CHECK(not index.find(DOCID(11), &offset));
    }
    // query by Title: miss
    {
        std::vector<offset_type> offsets;
        BOOST_CHECK(not index.find("Title 0", offsets));
        BOOST_CHECK(not index.find("Title 11", offsets));
    }
}

#if 0
/* Test serialization. */
BOOST_FIXTURE_TEST_CASE(test_serialization, TestFixture) {
    typedef scd::ScdIndex<test::Title> ScdIndex;
    typedef boost::scoped_ptr<ScdIndex> ScdIndexPtr;

    const unsigned NUM_DOC_A = 10;
    const unsigned NUM_DOC_B = 8;
    fs::path path_a = createScd("test_serialization_a.scd", NUM_DOC_A);
    fs::path path_b = createScd("test_serialization_b.scd", NUM_DOC_B, NUM_DOC_A + 1);

    // use smart pointer
    ScdIndexPtr index(ScdIndex::build(path_a.string()));
    BOOST_CHECK_EQUAL(12, ScdIndex::document_type::Version::value);

    // save to file
    fs::path saved = tempFile("saved-a");
    index->save(saved.string());
    BOOST_CHECK(fs::exists(saved));

    // load from file
    {
        ScdIndex loaded;
        loaded.load(saved.string());
        BOOST_CHECK_EQUAL(index->size(), loaded.size());
        for (ScdIndex::docid_iterator it = index->begin(); it != index->end(); ++it) {
            BOOST_CHECK(*it == *loaded.find(it->docid));
        }
    }

    // version mismatch
    {
        scd::ScdIndex<test::Content> loaded;
        BOOST_CHECK_EQUAL(14, scd::ScdIndex<test::Content>::document_type::Version::value);
        BOOST_CHECK_THROW(loaded.load(saved.string()), std::exception);
    }

    // loading from file _replaces_ existing content.
    {
#if DEBUG_OUTPUT
        std::cout << " * Before load:" << std::endl;
        std::copy(index->begin(), index->end(),
                  std::ostream_iterator<ScdIndex::document_type>(std::cout, "\n"));
#endif

        ScdIndexPtr index_b(ScdIndex::build(path_b.string()));
        fs::path saved_b = tempFile("saved-b");
        index_b->save(saved_b.string());

        index->load(saved_b.string());
#if DEBUG_OUTPUT
        std::cout << " * After load:" << std::endl;
        std::copy(index->begin(), index->end(),
                  std::ostream_iterator<ScdIndex::document_type>(std::cout, "\n"));
#endif
        BOOST_CHECK_EQUAL(index->size(), index_b->size());
        for (ScdIndex::docid_iterator it = index->begin(); it != index->end(); ++it) {
            BOOST_CHECK(*it == *index_b->find(it->docid));
        }
    }
}

/* Test uin128 wrapper type. */
BOOST_AUTO_TEST_CASE(test_uint) {
    { // docid as izenelib::util::UString
        const izenelib::util::UString docid = USTRING("f1667c41c7028665c198066b786bd149");

        const uint128 packed = str2uint(docid);
        const izenelib::util::UString unpacked = USTRING(uint2str(packed));
#if DEBUG_OUTPUT
        std::cout << "docid : " << docid << " -> " << sizeof(docid) << std::endl;
        std::cout << "pack  : " << packed << " -> " << sizeof(packed) << std::endl;
        std::cout << "unpack: " << unpacked << " -> " << sizeof(docid) << std::endl;
#endif
        BOOST_CHECK_EQUAL(docid, unpacked);
    }

    { // docid as std::string
        const std::string docid = "f1667c41c7028665c198066b786bd149";

        const uint128 packed = str2uint(docid);
        const std::string unpacked = uint2str(packed);
#if DEBUG_OUTPUT
        std::cout << "docid : " << docid << " -> " << sizeof(docid) << std::endl;
        std::cout << "pack  : " << packed << " -> " << sizeof(packed) << std::endl;
        std::cout << "unpack: " << unpacked << " -> " << sizeof(docid) << std::endl;
#endif
        BOOST_CHECK_EQUAL(docid, unpacked);
    }
 }

/* enable performance test on a big SCD */
#define TEST_SCD_INDEX_PERFORMANCE 0

#if TEST_SCD_INDEX_PERFORMANCE
namespace test {
SCD_INDEX_PROPERTY_TAG_TYPED(uuid, uint128); //< 'uuid' tag.
}

/* Index and serialize a _big_ SCD file. */
BOOST_FIXTURE_TEST_CASE(test_performance, TestFixture) {
#if 1
    // create a sample SCD file as defined in the fixture.
    typedef scd::ScdIndex<test::Title> ScdIndex;
    fs::path path = createScd("test_performance.scd", 21e5);
#else
    // load a real SCD file
    typedef scd::ScdIndex<test::uuid, scd::DOCID> ScdIndex;
    fs::path path = "/home/paolo/tmp/B-00-201207282137-29781-U-C.SCD";
    BOOST_REQUIRE(fs::exists(path));
#endif

    // measure elapsed time between a tic() and a toc()
    Timer timer;

    timer.tic();
    boost::scoped_ptr<ScdIndex> index(ScdIndex::build(path.string(), 2.5e4));
    timer.toc();
    std::cout << "\nIndexing time:\n\t" << timer.seconds() << " seconds" << std::endl;

    // save to file
    fs::path saved = tempFile("saved");
    timer.tic();
    index->save(saved.string());
    timer.toc();
    std::cout << "Serialization time:\n\t" << timer.seconds() << " seconds" << std::endl;
    BOOST_CHECK(fs::exists(saved));
    std::cout << "Archive size:\n\t" << fs::file_size(saved)/1024 << " KiB" << std::endl;

    // load from file
    {
        ScdIndex loaded;
        timer.tic();
        loaded.load(saved.string());
        timer.toc();
        std::cout << "Deserialization time:\n\t" << timer.seconds() << " seconds" << std::endl;
        BOOST_CHECK_EQUAL(index->size(), loaded.size());
        timer.tic();
        for (ScdIndex::docid_iterator it = index->begin(); it != index->end(); ++it) {
            BOOST_CHECK(*it == *loaded.find(it->docid));
        }
        timer.toc();
        std::cout << "Checking time:\n\t" << timer.seconds() << " seconds" << std::endl;
    }
}
#endif
#endif
