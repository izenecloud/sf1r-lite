/**
 * @file VisitManager.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef VISIT_MANAGER_H
#define VISIT_MANAGER_H

#include "RecTypes.h"
#include <sdb/SequentialDB.h>
#include <sdb/SDBCursorIterator.h>

#include <string>

#include <boost/serialization/set.hpp> // serialize ItemIdSet
#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * user's current session.
 */
struct VisitSession
{
    std::string sessionId_; /// current session id
    ItemIdSet itemSet_; /// the items visited in current session

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & sessionId_;
        ar & itemSet_;
    }
};

class RecommendMatrix;

class VisitManager
{
public:
    VisitManager(
        const std::string& visitDBPath,
        const std::string& recommendDBPath,
        const std::string& sessionDBPath
    );

    void flush();

    /**
     * Add visit item.
     * @param isRecItem true for recommended item
     * @return true for success, false for error happened.
     */
    bool addVisitItem(
        const std::string& sessionId,
        const std::string& userId,
        itemid_t itemId,
        bool isRecItem,
        RecommendMatrix* matrix
    );

    /**
     * Get @p itemIdSet visited by @p userId.
     * @param userId user id
     * @param itemidSet item id set visited by @p userId, it would be empty if @p userId
     *                  has not visited any item.
     * @return true for success, false for error happened.
     */
    bool getVisitItemSet(const std::string& userId, ItemIdSet& itemIdSet);

    /**
     * Get @p itemIdSet recommended to @p userId.
     * @param userId user id
     * @param itemidSet item id set recommended to @p userId before.
     * @return true for success, false for error happened.
     */
    bool getRecommendItemSet(const std::string& userId, ItemIdSet& itemIdSet);

    /**
     * Get the current visit session by @p userId.
     * @param userId user id
     * @param visitSession store the current visit session
     * @return true for success, false for error happened.
     */
    bool getVisitSession(const std::string& userId, VisitSession& visitSession);

    /**
     * The number of users who have visited items.
     */
    unsigned int visitUserNum();

    typedef izenelib::sdb::unordered_sdb_tc<std::string, ItemIdSet, ReadWriteLock> VisitDBType;
    typedef izenelib::sdb::SDBCursorIterator<VisitDBType> VisitIterator;
    VisitIterator begin();
    VisitIterator end();

private:
    bool updateVisitDB_(
        VisitDBType& db,
        const std::string& userId,
        itemid_t itemId
    );

    bool getVisitDB_(
        VisitDBType& db,
        const std::string& userId,
        ItemIdSet& itemIdSet
    );

    bool updateSessionDB_(
        const std::string& sessionId,
        const std::string& userId,
        itemid_t itemId,
        RecommendMatrix* matrix
    );

private:
    VisitDBType visitDB_; // the items visited
    VisitDBType recommendDB_; // the items recommended

    typedef izenelib::sdb::unordered_sdb_tc<std::string, VisitSession, ReadWriteLock> SessionDBType;
    SessionDBType sessionDB_; // the items in current session
};

} // namespace sf1r

#endif // VISIT_MANAGER_H
