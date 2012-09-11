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
#define GET(index, tag, value, field) scd::get<tag>(index, value)->field

BOOST_FIXTURE_TEST_CASE(test_index, TestFixture) {
    const unsigned DOC_NUM = 10;
    fs::path path = createScd("index", DOC_NUM);

    // build index
    ScdIndexer indexer;
    BOOST_REQUIRE_MESSAGE(indexer.build(path.string()), "Indexing failed");
    BOOST_CHECK_EQUAL(DOC_NUM, indexer.size());
    const scd::ScdIndex& expected = indexer.getIndex();
//    std::copy(expected.begin(), expected.end(), 
//              std::ostream_iterator<scd::Document<> >(std::cout, "\n"));
    
    // query: hit
    BOOST_CHECK_EQUAL(  0, GET(expected, scd::docid, DOCID(1), offset));
    BOOST_CHECK_EQUAL( 50, GET(expected, scd::docid, DOCID(2), offset));
    BOOST_CHECK_EQUAL( 93, GET(expected, scd::docid, DOCID(3), offset));
    BOOST_CHECK_EQUAL(136, GET(expected, scd::docid, DOCID(4), offset));
    BOOST_CHECK_EQUAL(179, GET(expected, scd::docid, DOCID(5), offset));
    BOOST_CHECK_EQUAL(222, GET(expected, scd::docid, DOCID(6), offset));
    BOOST_CHECK_EQUAL(265, GET(expected, scd::docid, DOCID(7), offset));
    BOOST_CHECK_EQUAL(308, GET(expected, scd::docid, DOCID(8), offset));
    BOOST_CHECK_EQUAL(351, GET(expected, scd::docid, DOCID(9), offset));
    BOOST_CHECK_EQUAL(394, GET(expected, scd::docid, DOCID(10), offset));

    // query: miss
    BOOST_CHECK(expected.end() == scd::get<scd::docid>(expected, DOCID(0)));
    BOOST_CHECK(expected.end() == scd::get<scd::docid>(expected, DOCID(11)));
    
    // save to file
    fs::path saved = tempFile("saved");
    indexer.save(saved.string());
    BOOST_CHECK(fs::exists(saved));
    
    // load from file
    {
        ScdIndexer si;
        si.load(saved.string());
        BOOST_CHECK_EQUAL(indexer.size(), si.size());
        const scd::ScdIndex& loaded = si.getIndex();
        BOOST_CHECK(std::equal(expected.begin(), expected.end(), loaded.begin()));
    }
}