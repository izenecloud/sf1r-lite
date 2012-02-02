#include "UserManagerTestFixture.h"
#include <recommend-manager/storage/UserManager.h>
#include <recommend-manager/User.h>

#include <boost/test/unit_test.hpp>

namespace
{

const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;

using sf1r::User;

void checkUser(const User& user1, const User& user2)
{
    BOOST_CHECK_EQUAL(user1.idStr_, user2.idStr_);

    User::PropValueMap::const_iterator it1 = user1.propValueMap_.begin();
    User::PropValueMap::const_iterator it2 = user2.propValueMap_.begin();
    for (; it1 != user1.propValueMap_.end() && it2 != user2.propValueMap_.end(); ++it1, ++it2)
    {
        BOOST_CHECK_EQUAL(it1->first, it2->first);
        BOOST_CHECK_EQUAL(it1->second, it2->second);
    }

    BOOST_CHECK(it1 == user1.propValueMap_.end());
    BOOST_CHECK(it2 == user2.propValueMap_.end());
}

}

namespace sf1r
{

void UserManagerTestFixture::resetInstance()
{
    // flush first
    userManager_.reset();
    userManager_.reset(factory_->createUserManager());
}

void UserManagerTestFixture::checkUserManager_()
{
    User user;
    for (vector<User>::const_iterator it = userVec_.begin();
        it != userVec_.end(); ++it)
    {
        BOOST_CHECK(userManager_->getUser(it->idStr_, user));
        checkUser(user, *it);
    }
}

void UserManagerTestFixture::checkAddUser()
{
    BOOST_TEST_MESSAGE("add user...");

    User user1;
    user1.idStr_ = "aaa_1";
    user1.propValueMap_["gender"].assign("男", ENCODING_TYPE);
    user1.propValueMap_["age"].assign("20", ENCODING_TYPE);
    user1.propValueMap_["job"].assign("student", ENCODING_TYPE);
    user1.propValueMap_["interest"].assign("reading", ENCODING_TYPE);

    userVec_.push_back(user1);
    BOOST_CHECK(userManager_->addUser(user1));
    BOOST_CHECK(userManager_->addUser(user1) == false); // duplicate add should fail

    User user2;
    user2.idStr_ = "aaa_51";
    user2.propValueMap_["gender"].assign("女", ENCODING_TYPE);
    user2.propValueMap_["age"].assign("30", ENCODING_TYPE);
    user2.propValueMap_["job"].assign("finance", ENCODING_TYPE);
    user2.propValueMap_["interest"].assign("movie", ENCODING_TYPE);

    userVec_.push_back(user2);
    BOOST_CHECK(userManager_->addUser(user2));

    User user3;
    BOOST_CHECK(userManager_->addUser(user3) == false); // empty user id
    BOOST_CHECK(userManager_->getUser("", user3) == false); // empty user id
    BOOST_CHECK(userManager_->getUser("bbb", user3) == false); // user id not exist

    checkUserManager_();

    userManager_->flush();
}

void UserManagerTestFixture::checkUpdateUser()
{
    if (userVec_.empty())
        return;

    BOOST_TEST_MESSAGE("update user...");

    User& user = userVec_.back();
    user.propValueMap_["gender"].assign("femal", ENCODING_TYPE);
    user.propValueMap_["age"].assign("33", ENCODING_TYPE);
    user.propValueMap_["job"].assign("lawyer", ENCODING_TYPE);
    user.propValueMap_["interest"].assign("shopping", ENCODING_TYPE);
    user.propValueMap_["born"].assign("上海", ENCODING_TYPE);

    BOOST_CHECK(userManager_->updateUser(user));

    checkUserManager_();
}

void UserManagerTestFixture::checkRemoveUser()
{
    if (userVec_.empty())
        return;

    BOOST_TEST_MESSAGE("remove user...");

    vector<User>::iterator userIt = userVec_.begin();
    std::string removeId = userIt->idStr_;
    userVec_.erase(userIt);

    BOOST_CHECK(userManager_->removeUser(removeId));

    User user;
    BOOST_CHECK(userManager_->getUser(removeId, user) == false);

    BOOST_CHECK(userManager_->removeUser("") == false); // empty user id
    BOOST_CHECK(userManager_->removeUser("ccc") == false); // user id not exist

    checkUserManager_();
}

} // namespace sf1r
