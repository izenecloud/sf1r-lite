/*
 * File:   t_ScdIndexTable.cpp
 * Author: Paolo D'Apice
 *
 * Created on September 24, 2012, 10:18 AM
 */

#include <boost/test/unit_test.hpp>

#include "common/ScdIndexTable.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

fs::path
temporaryFile(const std::string& name, bool deleteFlag = true) {
    fs::path file = fs::temp_directory_path()/name;
    if (deleteFlag) fs::remove_all(file);
    return file;
}

BOOST_AUTO_TEST_CASE(basic) {
    typedef scd::leveldb::DOCID DOCID;
    typedef scd::leveldb::uuid uuid;
    typedef scd::leveldb::ScdIndexTable<uuid, DOCID> ScdIndexTable;
    typedef scd::Document<DOCID, uuid> Document;

    fs::path file = temporaryFile("table");
    ScdIndexTable table(file.string());

    long offset;

    // insert
    BOOST_REQUIRE_NO_THROW(table.insert(Document(0l, "1", "42")));
    BOOST_REQUIRE_NO_THROW(offset = table.get("1"));
    BOOST_CHECK_EQUAL(0l, offset);

    // next insert _overwrite_ existing value
    BOOST_REQUIRE_NO_THROW(table.insert(Document(4l, "1", "71")));
    BOOST_REQUIRE_NO_THROW(offset = table.get("1"));
    BOOST_CHECK_EQUAL(4l, offset);

    // insert
    BOOST_REQUIRE_NO_THROW(table.insert(Document(12l, "2", "42")));
    BOOST_REQUIRE_NO_THROW(offset = table.get("2"));
    BOOST_CHECK_EQUAL(12l, offset);
}
