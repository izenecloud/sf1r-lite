#include "UserManager.h"

#include <glog/logging.h>

namespace sf1r
{

UserManager::UserManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

void UserManager::flush()
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

bool UserManager::addUser(const User& user)
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

bool UserManager::updateUser(const User& user)
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

bool UserManager::removeUser(const std::string& userId)
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

bool UserManager::getUser(const std::string& userId, User& user)
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

unsigned int UserManager::userNum()
{
    return container_.numItems();
}

UserManager::SDBIterator UserManager::begin()
{
    return SDBIterator(container_);
}

UserManager::SDBIterator UserManager::end()
{
    return SDBIterator();
}

} // namespace sf1r
