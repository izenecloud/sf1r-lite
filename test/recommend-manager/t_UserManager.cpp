///
/// @file t_UserManager.cpp
/// @brief test UserManager in User operations
/// @author Jun Jiang
/// @date Created 2011-04-18
///

#include "UserManagerTestFixture.h"
#include <recommend-manager/storage/UserManager.h>
#include <recommend-manager/User.h>

#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>

using namespace std;
using namespace sf1r;

namespace
{
const string TEST_DIR_STR = "recommend_test/t_UserManager";
const string KEYSPACE_NAME = "test_recommend";
const string COLLECTION_NAME = "example";
}

BOOST_AUTO_TEST_SUITE(UserManagerTest)

BOOST_FIXTURE_TEST_CASE(checkLocalUserManager, UserManagerTestFixture)
{
    BOOST_REQUIRE(initLocalStorage(COLLECTION_NAME, TEST_DIR_STR));

    resetInstance();
    checkAddUser();

    resetInstance();
    checkUpdateUser();
    checkRemoveUser();
}

BOOST_FIXTURE_TEST_CASE(checkRemoteUserManager, UserManagerTestFixture)
{
    if (! initRemoteStorage(COLLECTION_NAME, TEST_DIR_STR,
                            KEYSPACE_NAME, REMOTE_STORAGE_URL))
    {
        cerr << "warning: exit test case as failed to connect " << REMOTE_STORAGE_URL << endl;
        return;
    }

    resetInstance();
    checkAddUser();

    resetInstance();
    checkUpdateUser();
    checkRemoveUser();
}

BOOST_FIXTURE_TEST_CASE(checkCassandraNotConnect, UserManagerTestFixture)
{
    BOOST_REQUIRE(! initRemoteStorage(COLLECTION_NAME, TEST_DIR_STR,
                                      KEYSPACE_NAME, REMOTE_STORAGE_URL_NOT_CONNECT));

    resetInstance();

    User user;
    user.idStr_ = "aaa";

    BOOST_TEST_MESSAGE("add user...");
    BOOST_CHECK(userManager_->addUser(user) == false);
    BOOST_CHECK(userManager_->getUser(user.idStr_, user) == false);

    BOOST_TEST_MESSAGE("update user...");
    BOOST_CHECK(userManager_->updateUser(user) == false);
    BOOST_CHECK(userManager_->getUser(user.idStr_, user) == false);

    BOOST_TEST_MESSAGE("remove user...");
    BOOST_CHECK(userManager_->removeUser(user.idStr_) == false);
    BOOST_CHECK(userManager_->getUser(user.idStr_, user) == false);
}

BOOST_AUTO_TEST_SUITE_END() 
