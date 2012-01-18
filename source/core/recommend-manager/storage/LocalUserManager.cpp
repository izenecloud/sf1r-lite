#include "LocalUserManager.h"
#include <recommend-manager/User.h>

#include <glog/logging.h>

namespace sf1r
{

LocalUserManager::LocalUserManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

void LocalUserManager::flush()
{
    try
    {
        container_.flush();
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::flush(): " << e.what();
    }
}

bool LocalUserManager::addUser(const User& user)
{
    if (user.idStr_.empty())
        return false;

    bool result = false;
    try
    {
        result = container_.insertValue(user.idStr_, user);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::insertValue(): " << e.what();
    }

    return result;
}

bool LocalUserManager::updateUser(const User& user)
{
    if (user.idStr_.empty())
        return false;

    bool result = false;
    try
    {
        result = container_.update(user.idStr_, user);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return result;
}

bool LocalUserManager::removeUser(const std::string& userId)
{
    bool result = false;
    try
    {
        result = container_.del(userId);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::del(): " << e.what();
    }

    return result;
}

bool LocalUserManager::getUser(const std::string& userId, User& user)
{
    bool result = false;
    try
    {
        result = container_.getValue(userId, user);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
}

} // namespace sf1r
