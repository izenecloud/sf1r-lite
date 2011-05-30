#include "PurchaseManager.h"
#include "ItemManager.h"
#include "ItemIterator.h"

#include <boost/filesystem.hpp>
#include <glog/logging.h>

namespace sf1r
{

PurchaseManager::PurchaseManager(
    const std::string& path,
    ItemCFManager* itemCFManager,
    const ItemManager* itemManager
)
    : container_(path)
    , itemCFManager_(itemCFManager)
    , itemManager_(itemManager)
{
    container_.open();
}

PurchaseManager::~PurchaseManager()
{
    flush();
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
    const std::vector<itemid_t>& itemVec
)
{
    ItemIdSet itemIdSet;
    container_.getValue(userId, itemIdSet);

    std::list<sf1r::itemid_t> oldItems(itemIdSet.begin(), itemIdSet.end());
    std::list<sf1r::itemid_t> newItems;

    for (std::vector<itemid_t>::const_iterator it = itemVec.begin();
        it != itemVec.end(); ++it)
    {
        if (itemIdSet.insert(*it).second)
        {
            newItems.push_back(*it);
        }
    }

    // not purchased yet
    if (! newItems.empty())
    {
        try
        {
            if (container_.update(userId, itemIdSet) == false)
            {
                return false;
            }
        }
        catch(izenelib::util::IZENELIBException& e)
        {
            LOG(ERROR) << "exception in SDB::update(): " << e.what();
        }

        {
            ItemIterator itemIterator(1, itemManager_->maxItemId());
            itemCFManager_->incrementalBuild(userId, oldItems, newItems, itemIterator);
        }
    }

    return true;
}

bool PurchaseManager::getPurchaseItemSet(userid_t userId, ItemIdSet& itemIdSet)
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
