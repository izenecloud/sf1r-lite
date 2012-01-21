#include "RemoteUserManager.h"
#include <recommend-manager/User.h>

#include <glog/logging.h>
#include <vector>

namespace
{
const char* PROP_USERID = "USERID";

org::apache::cassandra::CfDef createCFDef(
    const std::string& keyspace,
    const std::string& columnFamily
)
{
    org::apache::cassandra::CfDef def;

    def.__set_keyspace(keyspace);
    def.__set_name(columnFamily);

    def.__set_key_validation_class("UTF8Type");
    def.__set_comparator_type("UTF8Type");
    def.__set_default_validation_class("UTF8Type");

    std::vector<org::apache::cassandra::ColumnDef> columns;

    org::apache::cassandra::ColumnDef gender;
    gender.__set_name("gender");
    gender.__set_validation_class("UTF8Type");
    columns.push_back(gender);

    org::apache::cassandra::ColumnDef age;
    age.__set_name("age");
    age.__set_validation_class("UTF8Type");
    columns.push_back(age);

    def.__set_column_metadata(columns);

    return def;
}

}

namespace sf1r
{

RemoteUserManager::RemoteUserManager(
    const std::string& keyspace,
    const std::string& columnFamily,
    libcassandra::Cassandra* client
)
    : cassandra_(createCFDef(keyspace, columnFamily), client)
{
}

bool RemoteUserManager::addUser(const User& user)
{
    // already exist
    User temp;
    if (getUser(user.idStr_, temp))
        return false;

    return updateUser(user);
}

bool RemoteUserManager::updateUser(const User& user)
{
    const std::string& key = user.idStr_;

    if (key.empty())
        return false;

    if (! cassandra_.insertColumn(key, PROP_USERID, key))
        return false;

    for (User::PropValueMap::const_iterator it = user.propValueMap_.begin();
        it != user.propValueMap_.end(); ++it)
    {
        const std::string& propName = it->first;
        std::string propValue;
        it->second.convertString(propValue, UString::UTF_8);

        if (! cassandra_.insertColumn(key, propName, propValue))
            return false;
    }

    return true;
}

bool RemoteUserManager::removeUser(const std::string& userId)
{
    // not exist
    User temp;
    if (! getUser(userId, temp))
        return false;

    return cassandra_.remove(userId);
}

bool RemoteUserManager::getUser(const std::string& userId, User& user)
{
    std::vector<org::apache::cassandra::Column> columns;
    if (! cassandra_.getColumns(userId, columns))
        return false;

    user.idStr_.clear();

    for (std::vector<org::apache::cassandra::Column>::const_iterator it = columns.begin();
        it != columns.end(); ++it)
    {
        const std::string& name = it->name;
        const std::string& value = it->value;

        if (name == PROP_USERID)
        {
            if (value != userId)
            {
                LOG(ERROR) << "the column value for USERID " << value << " is not equal to the key " << userId;
                return false;
            }
            user.idStr_ = value;
        }
        else
        {
            user.propValueMap_[name].assign(value, UString::UTF_8);
        }
    }

    if (user.idStr_.empty())
        return false;

    return true;
}

} // namespace sf1r
