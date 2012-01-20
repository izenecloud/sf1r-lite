#include "PurchaseManager.h"
#include "RecommendMatrix.h"

#include <glog/logging.h>

namespace sf1r
{

PurchaseManager::PurchaseManager(const std::string& path)
    : container_(path)
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
    const std::string& userId,
    const std::vector<itemid_t>& itemVec,
    RecommendMatrix* matrix
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

        if (matrix)
        {
            matrix->update(oldItems, newItems);
        }
    }

    return true;
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

} // namespace sf1r
