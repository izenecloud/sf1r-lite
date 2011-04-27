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

bool UserManager::addUser(userid_t userId, const User& user)
{
    bool result = false;
    try
    {
        result = container_.insertValue(userId, user);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::insertValue(): " << e.what();
    }

    return result;
}

bool UserManager::updateUser(userid_t userId, const User& user)
{
    bool result = false;
    try
    {
        result = container_.update(userId, user);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return result;
}

bool UserManager::removeUser(userid_t userId)
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

bool UserManager::getUser(userid_t userId, User& user)
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
