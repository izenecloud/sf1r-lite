/**
 * @file UserManager.h
 * @author Jun Jiang
 * @date 2011-04-18
 */

#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "RecTypes.h"
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

    bool addUser(userid_t userId, const User& user);
    bool updateUser(userid_t userId, const User& user);
    bool removeUser(userid_t userId);

    bool getUser(userid_t userId, User& user);
    unsigned int userNum();

    typedef izenelib::sdb::unordered_sdb_tc<userid_t, User, ReadWriteLock> SDBType;
    typedef izenelib::sdb::SDBCursorIterator<SDBType> SDBIterator;
    SDBIterator begin();
    SDBIterator end();

private:
    SDBType container_;
};

} // namespace sf1r

#endif // USER_MANAGER_H
