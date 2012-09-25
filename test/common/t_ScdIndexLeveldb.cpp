/*
 * File:   t_ScdIndexTable.cpp
 * Author: Paolo D'Apice
 *
 * Created on September 24, 2012, 10:18 AM
 */

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "common/ScdIndexLeveldb.h"
#include "common/ScdIndexTag.h"

namespace fs = boost::filesystem;

fs::path temporaryFile(const std::string& name, bool deleteFlag = true) {
    fs::path file = fs::temp_directory_path()/name;
    if (deleteFlag) fs::remove_all(file);
    return file;
}

/* enable print to stdout */
#define DEBUG_OUTPUT 1

#define print(X) std::cout << X << std::endl

BOOST_AUTO_TEST_CASE(basic) {
    typedef scd::ScdIndexLeveldb<scd::DOCID, scd::uuid> ScdIndexLeveldb;
    typedef scd::Document<scd::DOCID, scd::uuid> Document;

    fs::path docidDatabase = temporaryFile("scd-docid"),
             propertyDatabase = temporaryFile("scd-uuid");
    ScdIndexLeveldb table(docidDatabase.string(), propertyDatabase.string(), true);

    offset_type offset;
    std::vector<offset_type> offsets;

    // get
    BOOST_CHECK_EQUAL(0, table.size());
    BOOST_CHECK(not table.getByDocid(1, offset));
    BOOST_CHECK(not table.getByProperty(42, offsets));

    // insert
    table.insert(Document(0, 1, 42));
#if DEBUG_OUTPUT
    print("After insert:");
    table.dump();
#endif
    BOOST_CHECK_EQUAL(1, table.size());
    BOOST_CHECK(table.getByDocid(1, offset));
    BOOST_CHECK_EQUAL(0, offset);

    BOOST_CHECK(table.getByProperty(42, offsets));
    BOOST_CHECK_EQUAL(1, offsets.size());
    BOOST_CHECK_EQUAL(0, offsets[0]);

    // next insert _overwrite_ existing value
    table.insert(Document(4, 1, 71));
#if DEBUG_OUTPUT
    print("After insert duplicate:");
    table.dump();
#endif
    BOOST_CHECK_EQUAL(1, table.size());
    BOOST_CHECK(table.getByDocid(1, offset));
    BOOST_CHECK_EQUAL(4, offset);

    BOOST_CHECK(table.getByProperty(71, offsets));
    BOOST_CHECK_EQUAL(1, offsets.size());
    BOOST_CHECK_EQUAL(4, offsets[0]);
    // old property value is still there
    BOOST_CHECK(table.getByProperty(42, offsets));
    BOOST_CHECK_EQUAL(1, offsets.size());
    BOOST_CHECK_EQUAL(0, offsets[0]);

    // insert
    table.insert(Document(12, 2, 71));
#if DEBUG_OUTPUT
    print("After insert:");
    table.dump();
#endif
    BOOST_CHECK_EQUAL(2, table.size());
    BOOST_CHECK(table.getByDocid(2, offset));
    BOOST_CHECK_EQUAL(12, offset);

    BOOST_CHECK(table.getByProperty(71, offsets));
    BOOST_CHECK_EQUAL(2, offsets.size());
    BOOST_CHECK_EQUAL(4, offsets[0]);
    BOOST_CHECK_EQUAL(12, offsets[1]);
}
