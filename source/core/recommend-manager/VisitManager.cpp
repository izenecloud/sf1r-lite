#include "VisitManager.h"

namespace sf1r
{

VisitManager::VisitManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

void VisitManager::flush()
{
    container_.flush();
}

bool VisitManager::addVisitItem(userid_t userId, itemid_t itemId)
{
    ItemIdSet itemIdSet;
    container_.getValue(userId, itemIdSet);
    std::pair<ItemIdSet::iterator, bool> res = itemIdSet.insert(itemId);
    // not visited yet
    if (res.second)
    {
        return container_.update(userId, itemIdSet);
    }

    // already visited
    return true;
}

bool VisitManager::getVisitItemSet(userid_t userId, ItemIdSet& itemIdSet)
{
    return container_.getValue(userId, itemIdSet);
}

unsigned int VisitManager::visitUserNum()
{
    return container_.numItems();
}

VisitManager::SDBIterator VisitManager::begin()
{
    return SDBIterator(container_);
}

VisitManager::SDBIterator VisitManager::end()
{
    return SDBIterator();
}

} // namespace sf1r
