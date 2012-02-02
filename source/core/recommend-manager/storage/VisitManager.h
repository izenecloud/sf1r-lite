/**
 * @file VisitManager.h
 * @author Jun Jiang
 * @date 2012-01-29
 */

#ifndef VISIT_MANAGER_H
#define VISIT_MANAGER_H

#include <recommend-manager/RecTypes.h>

#include <string>

namespace sf1r
{

/**
 * user's current session.
 */
struct VisitSession
{
    std::string sessionId_; /// current session id
    ItemIdSet itemSet_; /// the items visited in current session
};

class RecommendMatrix;

class VisitManager
{
public:
    virtual ~VisitManager() {}

    virtual void flush() {}

    /**
     * Add visit item.
     * @return true for success, false for error happened.
     */
    bool addVisitItem(
        const std::string& sessionId,
        const std::string& userId,
        itemid_t itemId,
        RecommendMatrix* matrix
    );

    /**
     * @p userId visits @p itemId because that item was recommended to him,
     * such items could be fetched by @c getRecommendItemSet().
     * @param userId user id
     * @param itemId item id recommended to and visited by @p userId
     * @return true for success, false for error happened.
     */
    virtual bool visitRecommendItem(const std::string& userId, itemid_t itemId) = 0;

    /**
     * Get @p itemIdSet visited by @p userId.
     * @param userId user id
     * @param itemidSet item id set visited by @p userId, it would be empty if @p userId
     *                  has not visited any item.
     * @return true for success, false for error happened.
     */
    virtual bool getVisitItemSet(const std::string& userId, ItemIdSet& itemIdSet) = 0;

    /**
     * Get @p itemIdSet recommended to and visited by @p userId.
     * @param userId user id
     * @param itemIdSet item id set recommended to and visited by @p userId before.
     * @return true for success, false for error happened.
     */
    virtual bool getRecommendItemSet(const std::string& userId, ItemIdSet& itemIdSet) = 0;

    /**
     * Get the current visit session by @p userId.
     * @param userId user id
     * @param visitSession store the current visit session
     * @return true for success, false for error happened.
     */
    virtual bool getVisitSession(const std::string& userId, VisitSession& visitSession) = 0;

protected:
    /**
     * Save @p itemId visited by @p userId.
     * @return true for success, false for error happened.
     */
    virtual bool saveVisitItem_(const std::string& userId, itemid_t itemId) = 0;

    /**
     * Save the session items of @p userId.
     * @param userId user id
     * @param visitSession all session items visited by @p userId, including @p newItem
     * @param isNewSession true for @p visitSession.sessionId_ was changed, false for not changed
     * @param newItem the new item not visited by @p userId in current session
     * @return true for success, false for error happened.
     * @note the implementation could choose to save either @p visitSession
     *       or @p newItem according to its storage policy
     */
    virtual bool saveVisitSession_(
        const std::string& userId,
        const VisitSession& visitSession,
        bool isNewSession,
        itemid_t newItem
    ) = 0;

private:
    bool addSessionItemImpl_(
        const std::string& sessionId,
        const std::string& userId,
        itemid_t itemId,
        RecommendMatrix* matrix
    );
};

} // namespace sf1r

#endif // VISIT_MANAGER_H
