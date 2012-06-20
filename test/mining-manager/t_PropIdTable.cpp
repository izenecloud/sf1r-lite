///
/// @file t_PropIdTable.cpp
/// @brief test PropIdTable to store property value ids for each doc
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-05-31
///

#include "PropIdTableTestFixture.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(PropIdTableTest)

typedef sf1r::PropIdTableTestFixture<uint16_t, uint32_t> PropIdTestFixture;

BOOST_FIXTURE_TEST_CASE(checkPropId, PropIdTestFixture)
{
    checkIdList();

    appendIdList("123");
    appendIdList("");
    appendIdList("456 789");
    appendIdList("65535"); // 2^16-1
    appendIdList("65535 65534 65533");
    appendIdList("1 2 3 4 5 6 7 8 9");
    appendIdList("");
    appendIdList("");
    appendIdList("");
    appendIdList("9 7 5 3 1 2 4 6 8");
    appendIdList("10000 1000 100 10 1");

    checkIdList();
}

typedef sf1r::PropIdTableTestFixture<uint32_t, uint32_t> AttrIdTestFixture;

BOOST_FIXTURE_TEST_CASE(checkAttrId, AttrIdTestFixture)
{
    checkIdList();

    appendIdList("123");
    appendIdList("");
    appendIdList("456 789");
    appendIdList("2147483647"); // 2^31-1
    appendIdList("2147483647 2147483646 2147483645");
    appendIdList("1 2 3 4 5 6 7 8 9");
    appendIdList("");
    appendIdList("");
    appendIdList("");
    appendIdList("9 7 5 3 1 2 4 6 8");

    checkIdList();

    checkOverFlow();

    appendIdList("1000000000 100000000 10000000 1000000 100000 10000 1000 100 10 1");

    checkIdList();
}

BOOST_AUTO_TEST_SUITE_END() 
