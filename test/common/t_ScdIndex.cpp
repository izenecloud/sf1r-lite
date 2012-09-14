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
#include <boost/lexical_cast.hpp>

namespace fs = boost::filesystem;

struct TestFixture {
    TestFixture() {}
    fs::path createScd(const std::string& dir, const std::string& name,
            const unsigned size, const unsigned start = 1) const {
        std::cout << "Creating temporary SCD file with " 
                  << size << " documents ..." << std::endl;
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

SCD_INDEX_PROPERTY_TAG(Title)

typedef scd::ScdIndex<Title> ScdIndex;

bool compare(const ScdIndex::document_type& a, const ScdIndex::document_type& b) {
    return a.offset < b.offset;
}

BOOST_FIXTURE_TEST_CASE(test_index, TestFixture) {
    const unsigned DOC_NUM = 10;
    fs::path path = createScd("index", "test.scd", DOC_NUM);

    // build index
    ScdIndex* indexptr;
    BOOST_REQUIRE_NO_THROW(indexptr = ScdIndex::build(path.string()));

    // using const reference
    const ScdIndex& index = *indexptr;
    BOOST_CHECK_EQUAL(DOC_NUM, index.size());
#ifdef DEBUG_OUTPUT
    std::cout << "Index by DOCID: " << std::endl;
    std::copy(index.begin(), index.end(),
              std::ostream_iterator<ScdIndex::document_type>(std::cout, "\n"));

    std::cout << "Index by Title: " << std::endl;
    std::copy(index.begin<Title>(), index.end<Title>(),
              std::ostream_iterator<ScdIndex::document_type>(std::cout, "\n"));
#endif
    // expected values
    long offsets[] = { 0, 50, 93, 136, 179, 222, 265, 308, 351, 394 };

    // query: hit
    for (size_t i = 0; i < DOC_NUM; ++i) {
        std::string id = boost::lexical_cast<std::string>(i+1);
        BOOST_CHECK_EQUAL(offsets[i], index.find(UString(id))->offset);
        BOOST_CHECK_EQUAL(1, index.count(UString(id)));
    }
    for (size_t i = 0; i < DOC_NUM/2; ++i) {
        std::string title = "Title " + boost::lexical_cast<std::string>(i+1);
        BOOST_CHECK_EQUAL(offsets[i], index.find<Title>(UString(title))->offset);
        BOOST_CHECK_EQUAL(1, index.count<Title>(UString(title)));
    }

    // multiple values, unsorted
    BOOST_CHECK_EQUAL(5, index.count<Title>(UString("Title T")));
    ScdIndex::property_range range = index.equal_range<Title>(UString("Title T"));
    // copy to vector and sort for comparison
    std::vector<ScdIndex::document_type> documents(range.first, range.second);
    std::sort(documents.begin(), documents.end(), compare);
#ifdef DEBUG_OUTPUT
    std::cout << "documents with Title = \"" << UString("Title T") << "\"" << std::endl;
    std::copy(documents.begin(), documents.end(),
              std::ostream_iterator<ScdIndex::document_type>(std::cout, "\n"));
#endif
    for (size_t i = DOC_NUM/2, j = 0; i < DOC_NUM; ++i, ++j) {
        BOOST_CHECK_EQUAL(offsets[i], documents[j].offset);
    }

    // query: miss
    ScdIndex::docid_iterator docid_end = index.end();
    BOOST_CHECK(docid_end == index.find(UString("0")));
    BOOST_CHECK(docid_end == index.find(UString("11")));

    ScdIndex::property_iterator property_end = index.end<Title>();
    BOOST_CHECK(property_end == index.find<Title>(UString("Title 0")));
    BOOST_CHECK(property_end == index.find<Title>(UString("Title 11")));

    delete indexptr;
}

BOOST_FIXTURE_TEST_CASE(test_serialization, TestFixture) {
    fs::path path_a = createScd("serialization", "a.scd", 5);
    fs::path path_b = createScd("serialization", "b.scd", 3, 6);

    ScdIndex* index = ScdIndex::build(path_a.string());

    // save to file
    fs::path saved = tempFile("saved");
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

    // loading from file _replaces_ existing content.
    {
        ScdIndex* index_b = ScdIndex::build(path_b.string());
        fs::path saved_b = tempFile("saved-b");
        index_b->save(saved_b.string());

        index->load(saved_b.string());
#ifdef DEBUG_OUTPUT
        std::copy(index->begin(), index->end(),
                  std::ostream_iterator<ScdIndex::document_type>(std::cout, "\n"));
#endif
        BOOST_CHECK_EQUAL(index->size(), index_b->size());
        for (ScdIndex::docid_iterator it = index->begin(); it != index->end(); ++it) {
            BOOST_CHECK(*it == *index_b->find(it->docid));
        }
        delete index_b;
    }

    delete index;
}

SCD_INDEX_PROPERTY_TAG(uuid)

BOOST_FIXTURE_TEST_CASE(test_performance, TestFixture) {
#if 1
    fs::path path = createScd("serialization", "temp.scd", 21e5);
#else
    typedef scd::ScdIndex<Title> ScdIndex;
    fs::path path = "/home/paolo/tmp/B-00-201207282137-29781-U-C.SCD";
    BOOST_REQUIRE(fs::exists(path));
#endif

    Timer timer;

    timer.tic();    
    ScdIndex* index = ScdIndex::build(path.string(), 2.5e4);
    timer.toc();
    std::cout << "Indexing time: " << timer.seconds() << " seconds\n";
    
    // save to file
    fs::path saved = tempFile("saved");
    timer.tic();
    index->save(saved.string());
    timer.toc();
    std::cout << "Serialization time: " << timer.seconds() << " seconds\n";
    BOOST_CHECK(fs::exists(saved));
    std::cout << "Archive size: " << fs::file_size(saved)/1024 << " KiB\n";
    // load from file
    {
        ScdIndex loaded;
        timer.tic();
        loaded.load(saved.string());
        timer.toc();
        std::cout << "Deserialization time: " << timer.seconds() << " seconds\n";
        BOOST_CHECK_EQUAL(index->size(), loaded.size());
        timer.tic();
        for (ScdIndex::docid_iterator it = index->begin(); it != index->end(); ++it) {
            BOOST_CHECK(*it == *loaded.find(it->docid));
        }
        timer.toc();
        std::cout << "Checking time: " << timer.seconds() << " seconds\n";
    }
}