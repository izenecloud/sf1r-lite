/*
 * File:   t_ScdIndexTable.cpp
 * Author: Paolo D'Apice
 *
 * Created on September 24, 2012, 10:18 AM
 */

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#define private public
#include "common/ScdIndexTable.h"
#undef private

namespace fs = boost::filesystem;

fs::path temporaryFile(const std::string& name, bool deleteFlag = true) {
    fs::path file = fs::temp_directory_path()/name;
    if (deleteFlag) fs::remove_all(file);
    return file;
}

#define print(X) std::cout << X << std::endl

BOOST_AUTO_TEST_CASE(basic) {
    typedef scd::leveldb::DOCID DOCID;
    typedef scd::leveldb::uuid uuid;
    typedef scd::leveldb::ScdIndexTable<uuid, DOCID> ScdIndexTable;
    typedef scd::Document<DOCID, uuid> Document;

    fs::path docidDatabase = temporaryFile("scd-docid"),
             propertyDatabase = temporaryFile("scd-uuid");
    ScdIndexTable table(docidDatabase.string(), propertyDatabase.string());

    offset_type offset;
    std::vector<offset_type> offsets;

    // get
    BOOST_CHECK(not table.getByDocid("1", offset));
    BOOST_CHECK(not table.getByProperty("42", offsets));

    // insert
    table.insert(Document(0l, "1", "42"));
    print("After insert:");
    table.dump();

    BOOST_CHECK(table.getByDocid("1", offset));
    BOOST_CHECK_EQUAL(0l, offset);

    BOOST_CHECK(table.getByProperty("42", offsets));
    BOOST_CHECK_EQUAL(1, offsets.size());
    BOOST_CHECK_EQUAL(0l, offsets[0]);

    // next insert _overwrite_ existing value
    table.insert(Document(4l, "1", "71"));
    print("After insert:");
    table.dump();

    BOOST_CHECK(table.getByDocid("1", offset));
    BOOST_CHECK_EQUAL(4l, offset);

    BOOST_CHECK(table.getByProperty("71", offsets));
    BOOST_CHECK_EQUAL(1, offsets.size());
    BOOST_CHECK_EQUAL(4l, offsets[0]);
    // old property value is still there
    BOOST_CHECK(table.getByProperty("42", offsets));
    BOOST_CHECK_EQUAL(1, offsets.size());
    BOOST_CHECK_EQUAL(0l, offsets[0]);

    // insert
    table.insert(Document(12l, "2", "71"));
    print("After insert:");
    table.dump();

    BOOST_CHECK(table.getByDocid("2", offset));
    BOOST_CHECK_EQUAL(12l, offset);

    BOOST_CHECK(table.getByProperty("71", offsets));
    BOOST_CHECK_EQUAL(2, offsets.size());
    BOOST_CHECK_EQUAL(4l, offsets[0]);
    BOOST_CHECK_EQUAL(12l, offsets[1]);
}
