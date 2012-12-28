#include "PropSharedLockSet.h"
#include "PropSharedLock.h"

using namespace sf1r;

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
    if (lock == NULL)
        return;

    if (sharedLockSet_.insert(lock).second)
    {
        lock->lockShared();
    }
}
