#include "VisitManager.h"
#include <common/JobScheduler.h>

#include <glog/logging.h>

#include <list>

namespace
{

class CoVisitTask
{
public:
    CoVisitTask(
        sf1r::CoVisitManager* coVisitManager,
        const sf1r::ItemIdSet& totalIdSet,
        sf1r::itemid_t newId
    )
    : coVisitManager_(coVisitManager)
    {
        for (sf1r::ItemIdSet::const_iterator it = totalIdSet.begin();
            it != totalIdSet.end(); ++it)
        {
            if (*it != newId)
            {
                oldItems_.push_back(*it);
            }
            else
            {
                newItems_.push_back(*it);
            }
        }
    }

    CoVisitTask(const CoVisitTask& coVisitTask)
    : coVisitManager_(coVisitTask.coVisitManager_)
    {
        oldItems_.swap(coVisitTask.oldItems_);
        newItems_.swap(coVisitTask.newItems_);
    }

    void visit()
    {
        coVisitManager_->visit(oldItems_, newItems_);
    }

private:
    sf1r::CoVisitManager* coVisitManager_;
    mutable std::list<sf1r::itemid_t> oldItems_;
    mutable std::list<sf1r::itemid_t> newItems_;
};

}

namespace sf1r
{

VisitManager::VisitManager(
    const std::string& path,
    JobScheduler* jobScheduler,
    CoVisitManager* coVisitManager
)
    : container_(path)
    , jobScheduler_(jobScheduler)
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

        CoVisitTask task(coVisitManager_, itemIdSet, itemId);
        jobScheduler_->addTask(boost::bind(&CoVisitTask::visit, task));

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
        result = container_.getValue(userId, itemIdSet);
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
