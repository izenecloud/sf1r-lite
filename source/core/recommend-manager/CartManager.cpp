#include "CartManager.h"

#include <glog/logging.h>

namespace sf1r
{

CartManager::CartManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

CartManager::~CartManager()
{
    flush();
}

void CartManager::flush()
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

bool CartManager::updateCart(
    userid_t userId,
    const std::vector<itemid_t>& itemVec
)
{
    try
    {
        if (container_.update(userId, itemVec) == false)
        {
            return false;
        }
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return true;
}

bool CartManager::getCart(userid_t userId, std::vector<itemid_t>& itemVec)
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
