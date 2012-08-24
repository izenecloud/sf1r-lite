///
/// @file t_GroupManager.cpp
/// @brief test groupby functions
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-03-23
///
#include "GroupManagerTestFixture.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(GroupManagerTest)

BOOST_FIXTURE_TEST_CASE(checkGroupRep, sf1r::GroupManagerTestFixture)
{
    BOOST_TEST_MESSAGE("check empty group index");
    checkGetGroupRep();

    BOOST_TEST_MESSAGE("create group index 1st time");
    createDocument(100);
    checkGetGroupRep();

    BOOST_TEST_MESSAGE("load group index");
    resetGroupManager();
    checkGetGroupRep();

    BOOST_TEST_MESSAGE("create group index 2nd time");
    createDocument(1000);
    checkGetGroupRep();
}

BOOST_FIXTURE_TEST_CASE(rebuildGroupData, sf1r::GroupManagerTestFixture)
{
    BOOST_TEST_MESSAGE("config all GroupConfigs to rebuild type");
    configGroupPropRebuild();

    BOOST_TEST_MESSAGE("check empty group index");
    checkGetGroupRep();

    BOOST_TEST_MESSAGE("create group index 1st time");
    createDocument(100);
    checkGetGroupRep();

    BOOST_TEST_MESSAGE("create group index 2nd time");
    createDocument(200);
    checkGetGroupRep();

    BOOST_TEST_MESSAGE("load group index");
    resetGroupManager();
    checkGetGroupRep();

    BOOST_TEST_MESSAGE("create group index 3rd time");
    createDocument(300);
    checkGetGroupRep();
}

BOOST_AUTO_TEST_CASE(mergeGroupRep)
{
    sf1r::GroupManagerTestFixture::checkGroupRepMerge();
}

BOOST_AUTO_TEST_SUITE_END() 
