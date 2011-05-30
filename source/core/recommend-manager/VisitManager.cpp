#include "VisitManager.h"

#include <glog/logging.h>

#include <list>

namespace sf1r
{

VisitManager::VisitManager(
    const std::string& path,
    CoVisitManager* coVisitManager
)
    : container_(path)
    , coVisitManager_(coVisitManager)
{
    container_.open();
}

void VisitManager::flush()
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

bool VisitManager::addVisitItem(userid_t userId, itemid_t itemId)
{
    ItemIdSet itemIdSet;
    container_.getValue(userId, itemIdSet);

    std::pair<ItemIdSet::iterator, bool> res = itemIdSet.insert(itemId);
    // not visited yet
    if (res.second)
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

        if (result)
        {
            std::list<itemid_t> oldItems;
            std::list<itemid_t> newItems;

            for (ItemIdSet::const_iterator it = itemIdSet.begin();
                it != itemIdSet.end(); ++it)
            {
                if (*it != itemId)
                {
                    oldItems.push_back(*it);
                }
                else
                {
                    newItems.push_back(*it);
                }
            }

            coVisitManager_->visit(oldItems, newItems);
        }

        return result;
    }

    // already visited
    return true;
}

bool VisitManager::getVisitItemSet(userid_t userId, ItemIdSet& itemIdSet)
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
