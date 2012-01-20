#include "LocalPurchaseManager.h"

#include <glog/logging.h>

namespace sf1r
{

LocalPurchaseManager::LocalPurchaseManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

LocalPurchaseManager::~LocalPurchaseManager()
{
    flush();
}

void LocalPurchaseManager::flush()
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

bool LocalPurchaseManager::getPurchaseItemSet(const std::string& userId, ItemIdSet& itemIdSet)
{
    bool result = false;

    try
    {
        itemIdSet.clear();
        container_.getValue(userId, itemIdSet);
        result = true;
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
}

bool LocalPurchaseManager::savePurchaseItem_(
    const std::string& userId,
    const ItemIdSet& totalItems,
    const std::list<sf1r::itemid_t>& newItems
)
{
    bool result = false;

    try
    {
        result = container_.update(userId, totalItems);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    if (! result)
    {
        LOG(ERROR) << "error in savePurchaseItem_(), user id: " << userId
                   << ", total item num: " << totalItems.size();
    }

    return result;
}

} // namespace sf1r
