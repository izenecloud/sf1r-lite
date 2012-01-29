#include "VisitManager.h"
#include "RecommendMatrix.h"

#include <glog/logging.h>

#include <list>

namespace sf1r
{

VisitManager::VisitManager(
    const std::string& visitDBPath,
    const std::string& recommendDBPath,
    const std::string& sessionDBPath
)
    : visitDB_(visitDBPath)
    , recommendDB_(recommendDBPath)
    , sessionDB_(sessionDBPath)
{
    visitDB_.open();
    recommendDB_.open();
    sessionDB_.open();
}

void VisitManager::flush()
{
    try
    {
        visitDB_.flush();
        recommendDB_.flush();
        sessionDB_.flush();
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::flush(): " << e.what();
    }
}

bool VisitManager::addVisitItem(
    const std::string& sessionId,
    const std::string& userId,
    itemid_t itemId,
    bool isRecItem,
    RecommendMatrix* matrix
)
{
    if (updateVisitDB_(visitDB_, userId, itemId)
        && updateSessionDB_(sessionId, userId, itemId, matrix))
    {
        if (isRecItem
            && updateVisitDB_(recommendDB_, userId, itemId) == false)
        {
            return false;
        }

        return true;
    }

    return false;
}

bool VisitManager::getVisitItemSet(const std::string& userId, ItemIdSet& itemIdSet)
{
    return getVisitDB_(visitDB_, userId, itemIdSet);
}

bool VisitManager::getRecommendItemSet(const std::string& userId, ItemIdSet& itemIdSet)
{
    return getVisitDB_(recommendDB_, userId, itemIdSet);
}

bool VisitManager::getVisitSession(const std::string& userId, VisitSession& visitSession)
{
    bool result = false;
    try
    {
        sessionDB_.getValue(userId, visitSession);
        result = true;
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
}

bool VisitManager::updateVisitDB_(
    VisitDBType& db,
    const std::string& userId,
    itemid_t itemId
)
{
    ItemIdSet itemIdSet;
    db.getValue(userId, itemIdSet);

    std::pair<ItemIdSet::iterator, bool> res = itemIdSet.insert(itemId);
    // not visited yet
    if (res.second)
    {
        bool result = false;
        try
        {
            result = db.update(userId, itemIdSet);
        }
        catch(izenelib::util::IZENELIBException& e)
        {
            LOG(ERROR) << "exception in visit DB SDB::update(): " << e.what();
        }

        return result;
    }

    // already visited
    return true;
}

bool VisitManager::getVisitDB_(
    VisitDBType& db,
    const std::string& userId,
    ItemIdSet& itemIdSet
)
{
    bool result = false;
    try
    {
        itemIdSet.clear();
        db.getValue(userId, itemIdSet);
        result = true;
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
}

bool VisitManager::updateSessionDB_(
    const std::string& sessionId,
    const std::string& userId,
    itemid_t itemId,
    RecommendMatrix* matrix
)
{
    if (sessionId.empty())
    {
        LOG(ERROR) << "empty session id is not allowed to update session db, userId: " << userId
                   << ", itemId: " << itemId;
        return false;
    }

    VisitSession visitSession;
    sessionDB_.getValue(userId, visitSession);

    // reset session
    if (visitSession.sessionId_ != sessionId)
    {
        visitSession.sessionId_ = sessionId;
        visitSession.itemSet_.clear();
    }

    std::pair<ItemIdSet::iterator, bool> res = visitSession.itemSet_.insert(itemId);
    // not visited in current session
    if (res.second)
    {
        bool result = false;
        try
        {
            result = sessionDB_.update(userId, visitSession);
        }
        catch(izenelib::util::IZENELIBException& e)
        {
            LOG(ERROR) << "exception in visit session SDB::update(): " << e.what();
        }

        if (result)
        {
            std::list<itemid_t> oldItems;
            std::list<itemid_t> newItems;

            for (ItemIdSet::const_iterator it = visitSession.itemSet_.begin();
                it != visitSession.itemSet_.end(); ++it)
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

            if (matrix)
            {
                matrix->update(oldItems, newItems);
            }
        }

        return result;
    }

    // already visited in current session
    return true;
}

} // namespace sf1r
