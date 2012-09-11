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
    fs::path createScd(const std::string& name, const unsigned size) {
        fs::path path= createTempDir(name)/filename;
        ScdBuilder scd(path);
        for (unsigned i = 1; i <= size; ++i) {
            scd("DOCID") << i;
            scd("Title") << "Title " << i;
            scd("Content") << "Content " << i;
        }        
        return path;
    }
private:
    fs::path createTempDir(const std::string& name) {
        fs::path dir = fs::temp_directory_path()/name;
        fs::remove_all(dir);
        fs::create_directories(dir);
        return dir;
    }
    std::string filename;
};

BOOST_FIXTURE_TEST_CASE(test_index, TestFixture) {
    const unsigned DOC_NUM = 3;
    fs::path path = createScd("index", DOC_NUM);

    ScdIndexer indexer;
    BOOST_REQUIRE_MESSAGE(indexer.build(path.string()), "Indexing failed");
}