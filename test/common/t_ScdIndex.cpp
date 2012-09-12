/* 
 * File:   t_ScdIndex.cpp
 * Author: Paolo D'Apice
 *
 * Created on September 11, 2012, 10:02 AM
 */

#include <boost/test/unit_test.hpp>

#include "ScdBuilder.h"
#include "common/ScdIndexer.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

struct TestFixture {
    TestFixture() : filename("test.scd") {}
    fs::path createScd(const std::string& name, const unsigned size) const {
        fs::path path= createTempDir(name)/filename;
        ScdBuilder scd(path);
        for (unsigned i = 1; i <= size; ++i) {
            scd("DOCID") << i;
            scd("Title") << "Title " << i;
            scd("Content") << "Content " << i;
        }        
        return path;
    }
    fs::path tempFile(const std::string& name) const {
        fs::path file = fs::temp_directory_path()/name;
        fs::remove_all(file);
        BOOST_REQUIRE(not fs::exists(file));
        return file;
    }
private:
    fs::path createTempDir(const std::string& name) const {
        fs::path dir = fs::temp_directory_path()/name;
        fs::remove_all(dir);
        fs::create_directories(dir);
        BOOST_REQUIRE(fs::is_empty(dir));
        return dir;
    }
    std::string filename;
};

#define DOCID(X) izenelib::util::UString((#X), izenelib::util::UString::UTF_8)

BOOST_FIXTURE_TEST_CASE(test_index, TestFixture) {
    const unsigned DOC_NUM = 10;
    fs::path path = createScd("index", DOC_NUM);
    
    // build index
    ScdIndexer indexer;
    BOOST_REQUIRE_MESSAGE(indexer.build(path.string()), "Indexing failed");
    const scd::ScdIndex& expected = indexer.getIndex();
    BOOST_CHECK_EQUAL(DOC_NUM, expected.size());
    std::copy(expected.begin<scd::docid>(), expected.end<scd::docid>(), 
              std::ostream_iterator<scd::Document<> >(std::cout, "\n"));
    
    // query: hit
    BOOST_CHECK_EQUAL(  0, expected.find<scd::docid>(DOCID(1))->offset);
    BOOST_CHECK_EQUAL( 50, expected.find<scd::docid>(DOCID(2))->offset);
    BOOST_CHECK_EQUAL( 93, expected.find<scd::docid>(DOCID(3))->offset);
    BOOST_CHECK_EQUAL(136, expected.find<scd::docid>(DOCID(4))->offset);
    BOOST_CHECK_EQUAL(179, expected.find<scd::docid>(DOCID(5))->offset);
    BOOST_CHECK_EQUAL(222, expected.find<scd::docid>(DOCID(6))->offset);
    BOOST_CHECK_EQUAL(265, expected.find<scd::docid>(DOCID(7))->offset);
    BOOST_CHECK_EQUAL(308, expected.find<scd::docid>(DOCID(8))->offset);
    BOOST_CHECK_EQUAL(351, expected.find<scd::docid>(DOCID(9))->offset);
    BOOST_CHECK_EQUAL(394, expected.find<scd::docid>(DOCID(10))->offset);

    // query: miss
    scd::ScdIndex::docid_iterator end = expected.end<scd::docid>();
    BOOST_CHECK(end == expected.find<scd::docid>(DOCID(0)));
    BOOST_CHECK(end == expected.find<scd::docid>(DOCID(11)));

    delete index;
}

BOOST_FIXTURE_TEST_CASE(test_serialization, TestFixture) {
    const unsigned DOC_NUM = 10;
    fs::path path = createScd("index", DOC_NUM);

    ScdIndexer indexer;
    BOOST_REQUIRE_MESSAGE(indexer.build(path.string()), "Indexing failed");
    const scd::ScdIndex& index = indexer.getIndex();
    
    // save to file
    fs::path saved = tempFile("saved");
    index.save(saved.string());
    BOOST_CHECK(fs::exists(saved));
    
    // load from file
    {
        scd::ScdIndex loaded;
        loaded.load(saved.string());
        BOOST_CHECK_EQUAL(index.size(), loaded.size());
        BOOST_CHECK(std::equal(index.begin<scd::docid>(), index.end<scd::docid>(), 
                               loaded.begin<scd::docid>()));
    }
}