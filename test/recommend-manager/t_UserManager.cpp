///
/// @file t_UserManager.cpp
/// @brief test UserManager in User operations
/// @author Jun Jiang
/// @date Created 2011-04-18
///

#include <util/ustring/UString.h>
#include <recommend-manager/UserManager.h>

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
const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
const char* TEST_DIR_STR = "recommend_test/t_UserManager";
const char* USER_DB_STR = "user.db";
}

void checkUser(const User& user1, const User& user2)
{
    BOOST_CHECK_EQUAL(user1.idStr_, user2.idStr_);

    User::PropValueMap::const_iterator it1 = user1.propValueMap_.begin();
    User::PropValueMap::const_iterator it2 = user2.propValueMap_.begin();
    for (; it1 != user1.propValueMap_.end() && it2 != user2.propValueMap_.end(); ++it1, ++it2)
    {
        BOOST_CHECK_EQUAL(it1->second, it2->second);
    }

    BOOST_CHECK(it1 == user1.propValueMap_.end());
    BOOST_CHECK(it2 == user2.propValueMap_.end());
}

void checkUserManager(const vector<userid_t>& idVec, const vector<User>& userVec, UserManager& userManager)
{
    BOOST_CHECK_EQUAL(idVec.size(), userVec.size());
    BOOST_CHECK_EQUAL(userManager.userNum(), userVec.size());

    User user2;
    for (size_t i = 0; i < idVec.size(); ++i)
    {
        BOOST_CHECK(userManager.getUser(idVec[i], user2));
        checkUser(userVec[i], user2);
    }
}

void iterateUserManager(const vector<userid_t>& idVec, const vector<User>& userVec, UserManager& userManager)
{
    BOOST_CHECK_EQUAL(idVec.size(), userVec.size());
    BOOST_CHECK_EQUAL(userManager.userNum(), userVec.size());

    unsigned int iterNum = 0;
    for (UserManager::SDBIterator userIt = userManager.begin();
        userIt != userManager.end(); ++userIt)
    {
        bool isFind = false;
        for (size_t i = 0; i < idVec.size(); ++i)
        {
            if (idVec[i] == userIt->first)
            {
                checkUser(userIt->second, userVec[i]);
                isFind = true;
                break;
            }
        }
        BOOST_CHECK(isFind);
        ++iterNum;
    }

    BOOST_TEST_MESSAGE("iterNum: " << iterNum);
    BOOST_CHECK_EQUAL(iterNum, userManager.userNum());
}

BOOST_AUTO_TEST_SUITE(UserManagerTest)

BOOST_AUTO_TEST_CASE(checkUser)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

    bfs::path userPath(bfs::path(TEST_DIR_STR) / USER_DB_STR);

    vector<userid_t> idVec;
    vector<User> userVec;

    idVec.push_back(1);
    userVec.push_back(User());
    User& user1 = userVec.back();
    user1.idStr_ = "aaa_1";
    user1.propValueMap_["gender"].assign("男", ENCODING_TYPE);
    user1.propValueMap_["age"].assign("20", ENCODING_TYPE);
    user1.propValueMap_["job"].assign("student", ENCODING_TYPE);
    user1.propValueMap_["interest"].assign("reading", ENCODING_TYPE);

    idVec.push_back(51);
    userVec.push_back(User());
    User& user2 = userVec.back();
    user2.idStr_ = "aaa_51";
    user2.propValueMap_["gender"].assign("女", ENCODING_TYPE);
    user2.propValueMap_["age"].assign("30", ENCODING_TYPE);
    user2.propValueMap_["job"].assign("finance", ENCODING_TYPE);
    user2.propValueMap_["interest"].assign("movie", ENCODING_TYPE);

    {
        BOOST_TEST_MESSAGE("add user...");
        UserManager userManager(userPath.string());
        for (size_t i = 0; i < userVec.size(); ++i)
        {
            BOOST_CHECK(userManager.addUser(idVec[i], userVec[i]));
        }
        // duplicate add should fail
        BOOST_CHECK(userManager.addUser(idVec[0], userVec[0]) == false);

        checkUserManager(idVec, userVec, userManager);
        iterateUserManager(idVec, userVec, userManager);

        userManager.flush();
    }

    {
        BOOST_TEST_MESSAGE("update user...");
        UserManager userManager(userPath.string());

        checkUserManager(idVec, userVec, userManager);

        User& user2 = userVec.back();
        user2.propValueMap_["gender"].assign("femal", ENCODING_TYPE);
        user2.propValueMap_["age"].assign("33", ENCODING_TYPE);
        user2.propValueMap_["job"].assign("lawyer", ENCODING_TYPE);
        user2.propValueMap_["interest"].assign("shopping", ENCODING_TYPE);
        user2.propValueMap_["born"].assign("上海", ENCODING_TYPE);

        BOOST_CHECK(userManager.updateUser(idVec.back(), user2));
        checkUserManager(idVec, userVec, userManager);
        iterateUserManager(idVec, userVec, userManager);

        BOOST_TEST_MESSAGE("remove user...");
        userid_t removeId = idVec.front();
        BOOST_CHECK(userManager.removeUser(removeId));
        BOOST_CHECK(userManager.getUser(removeId, user2) == false);
        userVec.erase(userVec.begin());
        idVec.erase(idVec.begin());
        checkUserManager(idVec, userVec, userManager);
        iterateUserManager(idVec, userVec, userManager);

        userManager.flush();
    }

    {
        BOOST_TEST_MESSAGE("empty user...");
        UserManager userManager(userPath.string());

        checkUserManager(idVec, userVec, userManager);
        iterateUserManager(idVec, userVec, userManager);

        BOOST_CHECK(userManager.removeUser(idVec.front()));
        userVec.erase(userVec.begin());
        idVec.erase(idVec.begin());
        checkUserManager(idVec, userVec, userManager);
        iterateUserManager(idVec, userVec, userManager);

        userManager.flush();
    }
}

BOOST_AUTO_TEST_SUITE_END() 
