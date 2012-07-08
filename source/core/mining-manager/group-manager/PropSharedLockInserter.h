///
/// @file PropSharedLockInserter.h
/// @brief base class to insert PropSharedLock instance
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-06-25
///

#ifndef SF1R_PROP_SHARED_LOCK_INSERTER_H
#define SF1R_PROP_SHARED_LOCK_INSERTER_H

#include "../faceted-submanager/faceted_types.h"
#include "PropSharedLock.h"
#include <set>

NS_FACETED_BEGIN

class PropSharedLockInserter
{
public:
    virtual ~PropSharedLockInserter() {}

    typedef std::set<const PropSharedLock*> SharedLockSet;

    virtual void insertSharedLock(SharedLockSet& lockSet) const {}
};

NS_FACETED_END

#endif // SF1R_PROP_SHARED_LOCK_INSERTER_H
