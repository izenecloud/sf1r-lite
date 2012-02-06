#include "LocalCartManager.h"

#include <glog/logging.h>

namespace sf1r
{

LocalCartManager::LocalCartManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

void LocalCartManager::flush()
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

bool LocalCartManager::updateCart(
    const std::string& userId,
    const std::vector<itemid_t>& itemVec
)
{
    bool result = false;

    try
    {
        result = container_.update(userId, itemVec);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    if (! result)
    {
        LOG(ERROR) << "error in updateCart(), user id: " << userId
                   << ", cart item num: " << itemVec.size();
    }
    return result;
}

bool LocalCartManager::getCart(
    const std::string& userId,
    std::vector<itemid_t>& itemVec
)
{
    bool result = false;

    try
    {
        container_.getValue(userId, itemVec);
        result = true;
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
}

} // namespace sf1r
