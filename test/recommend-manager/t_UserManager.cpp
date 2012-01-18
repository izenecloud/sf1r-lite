///
/// @file t_UserManager.cpp
/// @brief test UserManager in User operations
/// @author Jun Jiang
/// @date Created 2011-04-18
///

#include "UserManagerTestFixture.h"
#include <recommend-manager/storage/LocalUserManager.h>
#include <recommend-manager/storage/RemoteUserManager.h>
#include <recommend-manager/User.h>
#include <recommend-manager/storage/CassandraAdaptor.h>
#include <log-manager/CassandraConnection.h>
#include <util/ustring/UString.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include <string>
#include <vector>

using namespace std;
using namespace boost;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const string TEST_DIR_STR = "recommend_test/t_UserManager";
const string USER_DB_STR = "user.db";

const string TEST_CASSANDRA_URL = "cassandra://localhost";
const string KEYSPACE_NAME = "test_recommend";
const string COLLECTION_NAME = "example";
const string COLUMN_FAMILY_NAME(COLLECTION_NAME + "_users");
}

BOOST_AUTO_TEST_SUITE(UserManagerTest)

BOOST_FIXTURE_TEST_CASE(checkLocalUserManager, UserManagerTestFixture)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

    bfs::path userPath(bfs::path(TEST_DIR_STR) / USER_DB_STR);

    {
        LocalUserManager userManager(userPath.string());
        setUserManager(&userManager);

        checkAddUser();
    }

    // load LocalUserManager from files
    LocalUserManager userManager(userPath.string());
    setUserManager(&userManager);

    checkUpdateUser();
    checkRemoveUser();
}

BOOST_FIXTURE_TEST_CASE(checkRemoteUserManager, UserManagerTestFixture)
{
    CassandraConnection& connection = CassandraConnection::instance();
    if (! connection.init(TEST_CASSANDRA_URL))
    {
        cerr << "warning: exit test case as failed to connect " << TEST_CASSANDRA_URL << endl;
        return;
    }

    {
        CassandraAdaptor client(KEYSPACE_NAME, "");
        client.dropColumnFamily(COLUMN_FAMILY_NAME);
    }

    RemoteUserManager userManager(KEYSPACE_NAME, COLLECTION_NAME);
    setUserManager(&userManager);

    checkAddUser();
    checkUpdateUser();
    checkRemoveUser();
}

BOOST_AUTO_TEST_CASE(checkCassandraNotConnect)
{
    CassandraConnection& connection = CassandraConnection::instance();
    const char* TEST_CASSANDRA_NOT_CONNECT_URL = "cassandra://localhost:9161";
    BOOST_CHECK(connection.init(TEST_CASSANDRA_NOT_CONNECT_URL) == false);

    RemoteUserManager userManager(KEYSPACE_NAME, COLLECTION_NAME);

    User user;
    user.idStr_ = "aaa_1";

    BOOST_TEST_MESSAGE("add user...");
    BOOST_CHECK(userManager.addUser(user) == false);
    BOOST_CHECK(userManager.getUser(user.idStr_, user) == false);

    BOOST_TEST_MESSAGE("update user...");
    BOOST_CHECK(userManager.updateUser(user) == false);
    BOOST_CHECK(userManager.getUser(user.idStr_, user) == false);

    BOOST_TEST_MESSAGE("remove user...");
    BOOST_CHECK(userManager.removeUser(user.idStr_) == false);
    BOOST_CHECK(userManager.getUser(user.idStr_, user) == false);
}

BOOST_AUTO_TEST_SUITE_END() 
