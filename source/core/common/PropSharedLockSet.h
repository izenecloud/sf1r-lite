///
/// @file PropSharedLockSet.h
/// @brief a set of PropSharedLock instances
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-08-21
///

#ifndef SF1R_PROP_SHARED_LOCK_SET_H
#define SF1R_PROP_SHARED_LOCK_SET_H

#include <set>

namespace sf1r
{
class PropSharedLock;

class PropSharedLockSet
{
public:
    ~PropSharedLockSet();

    void insertSharedLock(const PropSharedLock* lock);

private:
    typedef std::set<const PropSharedLock*> SharedLockSetT;
    SharedLockSetT sharedLockSet_;
};

} // namespace sf1r

#endif // SF1R_PROP_SHARED_LOCK_SET_H
