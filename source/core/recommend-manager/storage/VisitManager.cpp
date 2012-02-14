#include "VisitManager.h"
#include "RecommendMatrix.h"

#include <glog/logging.h>

#include <list>

namespace
{

using namespace sf1r;

void updateMatrix(
    const ItemIdSet& totalItems,
    itemid_t newItem,
    RecommendMatrix* matrix
)
{
    std::list<itemid_t> oldItems;
    for (ItemIdSet::const_iterator it = totalItems.begin();
        it != totalItems.end(); ++it)
    {
        if (*it != newItem)
        {
            oldItems.push_back(*it);
        }
    }

    std::list<itemid_t> newItems;
    newItems.push_back(newItem);

    matrix->update(oldItems, newItems);
}

}

namespace sf1r
{

bool VisitManager::addVisitItem(
    const std::string& sessionId,
    const std::string& userId,
    itemid_t itemId,
    RecommendMatrix* matrix
)
{
    if (saveVisitItem_(userId, itemId) &&
        addSessionItemImpl_(sessionId, userId, itemId, matrix))
    {
        return true;
    }

    LOG(ERROR) << "error in addVisitItem(), userId: " << userId
               << ", itemId: " << itemId
               << ", sessionId: " << sessionId;
    return false;
}

bool VisitManager::addSessionItemImpl_(
    const std::string& sessionId,
    const std::string& userId,
    itemid_t itemId,
    RecommendMatrix* matrix
)
{
    if (sessionId.empty())
    {
        LOG(ERROR) << "empty session id is not allowed to add session items, userId: " << userId
                   << ", itemId: " << itemId;
        return false;
    }

    VisitSession visitSession;
    if (! getVisitSession(userId, visitSession))
        return false;

    // reset session
    bool isNewSession = false;
    if (visitSession.sessionId_ != sessionId)
    {
        isNewSession = true;
        visitSession.sessionId_ = sessionId;
        visitSession.itemSet_.clear();
    }

    std::pair<ItemIdSet::iterator, bool> res = visitSession.itemSet_.insert(itemId);
    // not visited in current session
    if (res.second)
    {
        if (! saveVisitSession_(userId, visitSession, isNewSession, itemId))
            return false;

        if (matrix)
            updateMatrix(visitSession.itemSet_, itemId, matrix);
    }

    return true;
}

} // namespace sf1r
