/**
 * @file VisitManager.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef VISIT_MANAGER_H
#define VISIT_MANAGER_H

#include "RecTypes.h"
#include "SDBCursorIterator.h"
#include <sdb/SequentialDB.h>

#include <string>

#include <boost/serialization/set.hpp> // serialize ItemIdSet

namespace sf1r
{

class VisitManager
{
public:
    VisitManager(const std::string& path);

    void flush();

    bool addVisitItem(userid_t userId, itemid_t itemId);

    bool getVisitItemSet(userid_t userId, ItemIdSet& itemIdSet);
    /**
     * The number of users who have visited items.
     */
    unsigned int visitUserNum();

    typedef izenelib::sdb::unordered_sdb_tc<userid_t, ItemIdSet, ReadWriteLock> SDBType;
    typedef SDBCursorIterator<SDBType> SDBIterator;
    SDBIterator begin();
    SDBIterator end();

private:
    SDBType container_;
};

} // namespace sf1r

#endif // VISIT_MANAGER_H
