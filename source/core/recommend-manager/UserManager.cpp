#include "UserManager.h"

namespace sf1r
{

UserManager::UserManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

void UserManager::flush()
{
    container_.flush();
}

bool UserManager::addUser(const User& user)
{
    return container_.insertValue(user.id_, user);
}

bool UserManager::updateUser(const User& user)
{
    return container_.update(user.id_, user);
}

bool UserManager::removeUser(userid_t userId)
{
    return container_.del(userId);
}

bool UserManager::getUser(userid_t userId, User& user)
{
    return container_.getValue(userId, user);
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
