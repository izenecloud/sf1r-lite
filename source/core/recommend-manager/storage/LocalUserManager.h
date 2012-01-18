/**
 * @file LocalUserManager.h
 * @author Jun Jiang
 * @date 2011-04-18
 */

#ifndef LOCAL_USER_MANAGER_H
#define LOCAL_USER_MANAGER_H

#include "UserManager.h"
#include <sdb/SequentialDB.h>

#include <string>

namespace sf1r
{
struct User;

class LocalUserManager : public UserManager
{
public:
    LocalUserManager(const std::string& path);

    virtual void flush();

    virtual bool addUser(const User& user);

    virtual bool updateUser(const User& user);

    virtual bool removeUser(const std::string& userId);

    virtual bool getUser(const std::string& userId, User& user);

private:
    typedef izenelib::sdb::unordered_sdb_tc<std::string, User, ReadWriteLock> SDBType;
    SDBType container_;
};

} // namespace sf1r

#endif // LOCAL_USER_MANAGER_H
