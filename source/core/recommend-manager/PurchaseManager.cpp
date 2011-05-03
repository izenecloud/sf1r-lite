#include "PurchaseManager.h"
#include <common/JobScheduler.h>

#include <glog/logging.h>

namespace sf1r
{

PurchaseManager::PurchaseManager(
    const std::string& path,
    JobScheduler* jobScheduler,
    ItemCFManager* itemCFManager
)
    : container_(path)
    , jobScheduler_(jobScheduler)
    , itemCFManager_(itemCFManager)
{
    container_.open();
}

void PurchaseManager::flush()
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

bool PurchaseManager::addPurchaseItem(
    userid_t userId,
    const OrderItemVec& orderItemVec,
    const std::string& orderIdStr
)
{
    ItemIdSet itemIdSet;
    container_.getValue(userId, itemIdSet);

    const std::size_t oldNum = itemIdSet.size();
    for (OrderItemVec::const_iterator it = orderItemVec.begin();
        it != orderItemVec.end(); ++it)
    {
        itemIdSet.insert(it->itemId_);
    }

    // not purchased yet
    if (itemIdSet.size() > oldNum)
    {
        bool result = false;
        try
        {
            result = container_.update(userId, itemIdSet);
        }
        catch(izenelib::util::IZENELIBException& e)
        {
            LOG(ERROR) << "exception in SDB::update(): " << e.what();
        }

        return result;
    }

    // already purchased
    return true;
}

bool PurchaseManager::getPurchaseItemSet(userid_t userId, ItemIdSet& itemIdSet)
{
    bool result = false;
    try
    {
        result = container_.getValue(userId, itemIdSet);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
}

unsigned int PurchaseManager::purchaseUserNum()
{
    return container_.numItems();
}

PurchaseManager::SDBIterator PurchaseManager::begin()
{
    return SDBIterator(container_);
}

PurchaseManager::SDBIterator PurchaseManager::end()
{
    return SDBIterator();
}

} // namespace sf1r
