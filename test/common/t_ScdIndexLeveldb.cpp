/*
 * File:   t_ScdIndexTable.cpp
 * Author: Paolo D'Apice
 *
 * Created on September 24, 2012, 10:18 AM
 */

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

/* enable print to stdout */
#define DEBUG_OUTPUT 0

#define protected public
#include "common/ScdIndexLeveldb.h"
#undef protected
#include "common/ScdIndexTag.h"

namespace fs = boost::filesystem;

fs::path temporaryFile(const std::string& name, bool deleteFlag = true) {
    fs::path file = fs::temp_directory_path()/name;
    if (deleteFlag) fs::remove_all(file);
    return file;
}

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
    BOOST_REQUIRE_EQUAL(0, table.size());
    BOOST_CHECK(not table.getByDocid(1, offset));
    BOOST_CHECK(not table.getByProperty(42, offsets));

    // insert
    table.insert(Document(0, 1, 42));
#if DEBUG_OUTPUT
    print("After insert:");
    table.dump();
#endif
    BOOST_REQUIRE_EQUAL(1, table.size());
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
    BOOST_REQUIRE_EQUAL(1, table.size());
    BOOST_CHECK(table.getByDocid(1, offset));
    BOOST_CHECK_EQUAL(4, offset);

    BOOST_CHECK(table.getByProperty(71, offsets));
    BOOST_CHECK_EQUAL(1, offsets.size());
    BOOST_CHECK_EQUAL(4, offsets[0]);
    // old property value is still there
    BOOST_CHECK(table.getByProperty(42, offsets));
    BOOST_CHECK_EQUAL(1, offsets.size());
    BOOST_CHECK_EQUAL(0, offsets[0]);

    // insert batch
    table.insert(Document(12, 2, 71), true);
    table.insert(Document(35, 3, 45), true);
    table.insert(Document(45, 5, 42), true);
    // no insert until flush
    BOOST_CHECK_EQUAL(1, table.size());
#if DEBUG_OUTPUT
    print("Before flush:");
    table.dumpBuffer();
#endif

    table.flush();
#if DEBUG_OUTPUT
    print("After flush:");
    table.dump();
    table.dumpBuffer();
#endif
    BOOST_REQUIRE_EQUAL(4, table.size());
    BOOST_CHECK(table.getByDocid(2, offset));
    BOOST_CHECK_EQUAL(12, offset);
    BOOST_CHECK(table.getByDocid(3, offset));
    BOOST_CHECK_EQUAL(35, offset);
    BOOST_CHECK(table.getByDocid(5, offset));
    BOOST_CHECK_EQUAL(45, offset);

    BOOST_CHECK(table.getByProperty(71, offsets));
    BOOST_CHECK_EQUAL(2, offsets.size());
    BOOST_CHECK_EQUAL(4, offsets[0]);
    BOOST_CHECK_EQUAL(12, offsets[1]);
    BOOST_CHECK(table.getByProperty(45, offsets));
    BOOST_CHECK_EQUAL(1, offsets.size());
    BOOST_CHECK_EQUAL(35, offsets[0]);
    BOOST_CHECK(table.getByProperty(42, offsets));
    BOOST_CHECK_EQUAL(2, offsets.size());
    BOOST_CHECK_EQUAL(0, offsets[0]);
    BOOST_CHECK_EQUAL(45, offsets[1]);
}
