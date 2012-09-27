/*
 * File:   t_ScdIndexTable.cpp
 * Author: Paolo D'Apice
 *
 * Created on September 24, 2012, 10:18 AM
 */

#include "ScdIndexUtils.hpp"
#include <boost/test/unit_test.hpp>

/* enable print to stdout */
#define DEBUG_OUTPUT 0

#define protected public
#include "common/ScdIndexLeveldb.h"
#undef protected
#include "common/ScdIndexTag.h"

#define print(X) std::cout << X << std::endl

BOOST_AUTO_TEST_CASE(basic) {
    typedef scd::ScdIndexLeveldb<scd::DOCID, scd::uuid> ScdIndexLeveldb;
    typedef scd::Document<scd::DOCID, scd::uuid> Document;

    fs::path docidDatabase = test::createTempFile("scd-docid"),
             propertyDatabase = test::createTempFile("scd-uuid");
    ScdIndexLeveldb table(docidDatabase.string(), propertyDatabase.string(), true);

    offset_type offset;
    offset_list offsets;

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

    // flush
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

    // Docid iterator
    const scd::DOCID::type expected_id[] = { 1, 2, 3, 5 };
    const offset_type expected_offset[] = { 4, 12, 35, 45 };
    int j = 0;
    for (ScdIndexLeveldb::DocidIterator it = table.begin(); it != table.end(); ++it, ++j) {
        BOOST_CHECK(expected_id[j] == it->first);
        BOOST_CHECK(expected_offset[j] == it->second);
    }

    // Property iterator
    const scd::uuid::type expected_uuid[] = { 42, 45, 71 };
    offset_list l1, l2, l3;
    l1.push_back(0); l1.push_back(45);
    l2.push_back(35);
    l3.push_back(4); l3.push_back(12);
    const offset_list expected_list[] = { l1, l2, l3 };
    j = 0;
    for (ScdIndexLeveldb::PropertyIterator it = table.pbegin(); it != table.pend(); ++it, ++j) {
        BOOST_CHECK(expected_uuid[j] == it->first);
        BOOST_CHECK_EQUAL(expected_list[j].size(), it->second.size());
        for (unsigned i = 0; i < expected_list[j].size(); ++i)
            BOOST_CHECK_EQUAL(expected_list[j][i], it->second[i]);
    }
}
