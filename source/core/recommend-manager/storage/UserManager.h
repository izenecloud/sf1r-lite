/**
 * @file UserManager.h
 * @author Jun Jiang
 * @date 2011-04-18
 */

#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <string>

namespace sf1r
{
struct User;

class UserManager
{
public:
    virtual ~UserManager() {}

    virtual void flush() {}

    virtual bool addUser(const User& user) = 0;

    virtual bool updateUser(const User& user) = 0;

    virtual bool removeUser(const std::string& userId) = 0;

    virtual bool getUser(const std::string& userId, User& user) = 0;
};

} // namespace sf1r

#endif // USER_MANAGER_H
