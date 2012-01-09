/**
 * @file UserManager.h
 * @author Jun Jiang
 * @date 2011-04-18
 */

#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "User.h"
#include <sdb/SequentialDB.h>
#include <sdb/SDBCursorIterator.h>

#include <string>

namespace sf1r
{

class UserManager
{
public:
    UserManager(const std::string& path);

    void flush();

    bool addUser(const User& user);
    bool updateUser(const User& user);
    bool removeUser(const std::string& userId);

    bool getUser(const std::string& userId, User& user);
    unsigned int userNum();

    typedef izenelib::sdb::unordered_sdb_tc<std::string, User, ReadWriteLock> SDBType;
    typedef izenelib::sdb::SDBCursorIterator<SDBType> SDBIterator;
    SDBIterator begin();
    SDBIterator end();

private:
    SDBType container_;
};

} // namespace sf1r

#endif // USER_MANAGER_H
