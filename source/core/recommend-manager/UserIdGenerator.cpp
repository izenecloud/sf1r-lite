#include "UserIdGenerator.h"

namespace sf1r
{

UserIdGenerator::UserIdGenerator(const std::string& path)
    : generator_(path)
{
}

void UserIdGenerator::flush()
{
    generator_.flush();
}

userid_t UserIdGenerator::insert(const std::string& strId)
{
    userid_t userId = 0;
    generator_.conv(strId, userId, true);

    return userId;
}

bool UserIdGenerator::get(const std::string& strId, userid_t& userId)
{
    if (generator_.conv(strId, userId, false))
        return true;

    LOG(WARNING) << "in get(), user id " << strId << " not yet inserted before";
    return false;
}

}
