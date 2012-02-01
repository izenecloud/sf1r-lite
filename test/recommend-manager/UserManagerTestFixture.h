///
/// @file UserManagerTestFixture.h
/// @brief fixture class to test UserManager
/// @author Jun Jiang
/// @date Created 2011-11-08
///

#ifndef ITEM_MANAGER_TEST_FIXTURE
#define ITEM_MANAGER_TEST_FIXTURE

#include "RecommendStorageTestFixture.h"

#include <vector>

namespace sf1r
{
class UserManager;
struct User;

class UserManagerTestFixture : public RecommendStorageTestFixture
{
public:
    virtual void resetInstance();

    void checkAddUser();

    void checkUpdateUser();

    void checkRemoveUser();

private:
    void checkUserManager_();

protected:
    boost::scoped_ptr<UserManager> userManager_;

    std::vector<User> userVec_;
};

} // namespace sf1r

#endif // ITEM_MANAGER_TEST_FIXTURE
