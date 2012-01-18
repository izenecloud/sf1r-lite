#include "PurchaseManager.h"

#include <boost/filesystem.hpp>
#include <glog/logging.h>

namespace sf1r
{

PurchaseManager::PurchaseManager(
    const std::string& path,
    ItemCFManager& itemCFManager
)
    : container_(path)
    , itemCFManager_(itemCFManager)
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
        itemCFManager_.flush();
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::flush(): " << e.what();
    }
}

bool PurchaseManager::addPurchaseItem(
    const std::string& userId,
    const std::vector<itemid_t>& itemVec,
    bool isUpdateSimMatrix
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
        bool result = false;
        try
        {
            result = container_.update(userId, itemIdSet);
        }
        catch(izenelib::util::IZENELIBException& e)
        {
            LOG(ERROR) << "exception in SDB::update(): " << e.what();
        }

        if (!result)
        {
            LOG(ERROR) << "error in addPurchaseItem(), user id: " << userId
                       << ", item num: " << itemIdSet.size();
            return false;
        }

        if (isUpdateSimMatrix)
        {
            itemCFManager_.updateMatrix(oldItems, newItems);
        }
        else
        {
            itemCFManager_.updateVisitMatrix(oldItems, newItems);
        }
    }

    return true;
}

void PurchaseManager::buildSimMatrix()
{
    itemCFManager_.buildSimMatrix();
}

bool PurchaseManager::getPurchaseItemSet(const std::string& userId, ItemIdSet& itemIdSet)
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

void PurchaseManager::print(std::ostream& ostream) const
{
    ostream << "[Purchase] " << itemCFManager_;
}

std::ostream& operator<<(std::ostream& out, const PurchaseManager& purchaseManager)
{
    purchaseManager.print(out);
    return out;
}

} // namespace sf1r
