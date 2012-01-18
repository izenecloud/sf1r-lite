/**
 * @file RemoteUserManager.h
 * @author Jun Jiang
 * @date 2012-01-10
 */

#ifndef REMOTE_USER_MANAGER_H
#define REMOTE_USER_MANAGER_H

#include "UserManager.h"
#include "CassandraAdaptor.h"

#include <3rdparty/libcassandra/cassandra.h>

#include <string>

namespace sf1r
{
struct User;

class RemoteUserManager : public UserManager
{
public:
    RemoteUserManager(const std::string& keyspace, const std::string& collection);

    bool addUser(const User& user);

    bool updateUser(const User& user);

    bool removeUser(const std::string& userId);

    bool getUser(const std::string& userId, User& user);

private:
    void createCFDef_(org::apache::cassandra::CfDef& def) const;

private:
    const std::string keyspace_;
    const std::string columnFamily_;
    CassandraAdaptor cassandra_;
};

} // namespace sf1r

#endif // REMOTE_USER_MANAGER_H
