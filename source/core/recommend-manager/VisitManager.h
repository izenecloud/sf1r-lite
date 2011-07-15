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

class VisitManager
{
public:
    VisitManager(
        const std::string& visitDBPath,
        const std::string& sessionDBPath,
        CoVisitManager* coVisitManager
    );

    void flush();

    bool addVisitItem(
        const std::string& sessionId,
        userid_t userId,
        itemid_t itemId
    );

    /**
     * Get @p itemIdSet visited by @p userId.
     * @param userId user id
     * @param itemidSet item id set visited by @p userId, it would be empty if @p userId
     *                  has not visited any item.
     * @return true for success, false for error happened.
     */
    bool getVisitItemSet(userid_t userId, ItemIdSet& itemIdSet);

    /**
     * The number of users who have visited items.
     */
    unsigned int visitUserNum();

    typedef izenelib::sdb::unordered_sdb_tc<userid_t, ItemIdSet, ReadWriteLock> SDBType;
    typedef izenelib::sdb::SDBCursorIterator<SDBType> SDBIterator;
    SDBIterator begin();
    SDBIterator end();

private:
    bool updateVisitDB_(
        userid_t userId,
        itemid_t itemId
    );

    bool updateSessionDB_(
        const std::string& sessionId,
        userid_t userId,
        itemid_t itemId
    );

private:
    SDBType visitDB_;

    typedef izenelib::sdb::unordered_sdb_tc<userid_t, VisitSession, ReadWriteLock> SessionDBType;
    SessionDBType sessionDB_;

    CoVisitManager* coVisitManager_;
};

} // namespace sf1r

#endif // VISIT_MANAGER_H
