#include "LocalUserManager.h"
#include "../common/User.h"

#include <node-manager/RequestLog.h>
#include <node-manager/DistributeRequestHooker.h>
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
    if (!DistributeRequestHooker::get()->isValid())
    {
        LOG(ERROR) << __FUNCTION__ << " call invalid.";
        return false;
    }

    if (user.idStr_.empty())
    {
        DistributeRequestHooker::get()->processLocalFinished(false);
        return false;
    }

    NoAdditionNoRollbackReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataNoRollback, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool result = false;
    try
    {
        result = container_.insertValue(user.idStr_, user);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::insertValue(): " << e.what();
    }

    DistributeRequestHooker::get()->processLocalFinished(result);
    return result;
}

bool LocalUserManager::updateUser(const User& user)
{
    if (!DistributeRequestHooker::get()->isValid())
    {
        LOG(ERROR) << __FUNCTION__ << " call invalid.";
        return false;
    }

    if (user.idStr_.empty())
    {
        DistributeRequestHooker::get()->processLocalFinished(false);
        return false;
    }

    bool result = false;
    NoAdditionNoRollbackReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataNoRollback, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    try
    {
        result = container_.update(user.idStr_, user);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    DistributeRequestHooker::get()->processLocalFinished(result);
    return result;
}

bool LocalUserManager::removeUser(const std::string& userId)
{
    if (!DistributeRequestHooker::get()->isValid())
    {
        LOG(ERROR) << __FUNCTION__ << " call invalid.";
        return false;
    }
    NoAdditionNoRollbackReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataNoRollback, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool result = false;
    try
    {
        result = container_.del(userId);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::del(): " << e.what();
    }

    DistributeRequestHooker::get()->processLocalFinished(result);
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
