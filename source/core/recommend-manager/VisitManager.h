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

namespace sf1r
{
class JobScheduler;

class VisitManager
{
public:
    VisitManager(
        const std::string& path,
        JobScheduler* jobScheduler,
        CoVisitManager* coVisitManager
    );

    void flush();

    bool addVisitItem(userid_t userId, itemid_t itemId);

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
    SDBType container_;
    JobScheduler* jobScheduler_;
    CoVisitManager* coVisitManager_;
};

} // namespace sf1r

#endif // VISIT_MANAGER_H
