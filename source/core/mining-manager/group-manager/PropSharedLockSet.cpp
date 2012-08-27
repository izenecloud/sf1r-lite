#include "PropSharedLockSet.h"
#include "PropSharedLock.h"

NS_FACETED_BEGIN

PropSharedLockSet::~PropSharedLockSet()
{
    for (SharedLockSetT::const_iterator it = sharedLockSet_.begin();
        it != sharedLockSet_.end(); ++it)
    {
        (*it)->unlockShared();
    }
}

void PropSharedLockSet::insertSharedLock(const PropSharedLock* lock)
{
    if (sharedLockSet_.insert(lock).second)
    {
        lock->lockShared();
    }
}

NS_FACETED_END
