/* 
 * File:   t_ScdIndex.cpp
 * Author: Paolo D'Apice
 *
 * Created on September 11, 2012, 10:02 AM
 */

#include <boost/test/unit_test.hpp>

#include "ScdBuilder.h"
#include "common/ScdIndex.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

namespace fs = boost::filesystem;

struct TestFixture {
    TestFixture() {}
    fs::path createScd(const std::string& dir, const std::string& name, 
            const unsigned size, const unsigned start = 1) const {
        fs::path path = createTempDir(dir)/name;
        ScdBuilder scd(path);
        for (unsigned i = start; i < start + size; ++i) {
            scd("DOCID") << i;
            if (i > (start+size)/2)
                scd("Title") << "Title T";
            else 
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
        fs::create_directories(dir);
        return dir;
    }
};

#define UString(X) izenelib::util::UString((X), izenelib::util::UString::UTF_8)

SCD_INDEX_PROPERTY_TAG(Title, "Title")

BOOST_FIXTURE_TEST_CASE(test_index, TestFixture) {
    const unsigned DOC_NUM = 10;
    fs::path path = createScd("index", "test.scd", DOC_NUM);
    
    // build index
    scd::ScdIndex<Title>* indexptr;
    BOOST_REQUIRE_NO_THROW(indexptr = scd::ScdIndex<Title>::build(path.string()));

    // using const reference
    const scd::ScdIndex<Title>& index = *indexptr;
    BOOST_CHECK_EQUAL(DOC_NUM, index.size());

    std::cout << "Index by DOCID: " << std::endl;
    std::copy(index.begin<scd::docid>(), index.end<scd::docid>(), 
              std::ostream_iterator<scd::Document<Title> >(std::cout, "\n"));

    std::cout << "Index by property: " << std::endl;
    std::copy(index.begin<Title>(), index.end<Title>(), 
              std::ostream_iterator<scd::Document<Title> >(std::cout, "\n"));

    // expected values
    long offsets[] = { 0, 50, 93, 136, 179, 222, 265, 308, 351, 394 };

    // query: hit
    for (size_t i = 0; i < DOC_NUM; ++i) {
        std::string id = boost::lexical_cast<std::string>(i+1);
        BOOST_CHECK_EQUAL(offsets[i], index.find<scd::docid>(UString(id))->offset);
        BOOST_CHECK_EQUAL(1, index.count<scd::docid>(UString(id)));
    }
    for (size_t i = 0; i < DOC_NUM/2; ++i) {
        std::string title = "Title " + boost::lexical_cast<std::string>(i+1);
        BOOST_CHECK_EQUAL(offsets[i], index.find<Title>(UString(title))->offset);
        BOOST_CHECK_EQUAL(1, index.count<Title>(UString(title))); 
    }
    BOOST_CHECK_EQUAL(5, index.count<Title>(UString("Title T"))); 
    scd::ScdIndex<Title>::property_iterator it = index.find<Title>(UString("Title T"));
    for (size_t i = DOC_NUM/2; i < DOC_NUM; ++i) {
        BOOST_CHECK_EQUAL(offsets[i], (it++)->offset);
    }
    BOOST_CHECK(index.end<Title>() == it);
    
    // query: miss
    scd::ScdIndex<Title>::docid_iterator docid_end = index.end<scd::docid>();
    BOOST_CHECK(docid_end == index.find<scd::docid>(UString("0")));
    BOOST_CHECK(docid_end == index.find<scd::docid>(UString("11")));

    scd::ScdIndex<Title>::property_iterator property_end = index.end<Title>();
    BOOST_CHECK(property_end == index.find<Title>(UString("Title 0")));
    BOOST_CHECK(property_end == index.find<Title>(UString("Title 11")));

    delete indexptr;
}

BOOST_FIXTURE_TEST_CASE(test_serialization, TestFixture) {
    fs::path path_a = createScd("serialization", "a.scd", 5);
    fs::path path_b = createScd("serialization", "b.scd", 3, 5);

    scd::ScdIndex<Title>* index = scd::ScdIndex<Title>::build(path_a.string());
    
    // save to file
    fs::path saved = tempFile("saved");
    index->save(saved.string());
    BOOST_CHECK(fs::exists(saved));
    
    // load from file
    {
        scd::ScdIndex<Title> loaded;
        loaded.load(saved.string());
        BOOST_CHECK_EQUAL(index->size(), loaded.size());
        BOOST_CHECK(std::equal(index->begin<scd::docid>(), index->end<scd::docid>(), 
                               loaded.begin<scd::docid>()));
    }

    // loading from file _replaces_ existing content.
    {
        scd::ScdIndex<Title>* index_b = scd::ScdIndex<Title>::build(path_b.string());
        fs::path saved_b = tempFile("saved-b");
        index_b->save(saved_b.string());

        index->load(saved_b.string());
        std::copy(index->begin<scd::docid>(), index->end<scd::docid>(), 
                  std::ostream_iterator<scd::Document<Title> >(std::cout, "\n"));

        BOOST_CHECK_EQUAL(index->size(), index_b->size());
        BOOST_CHECK(std::equal(index->begin<scd::docid>(), index->end<scd::docid>(), 
                               index_b->begin<scd::docid>()));
        delete index_b;
    }
    
    delete index;
}