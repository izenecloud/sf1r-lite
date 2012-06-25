///
/// @file PropSharedLockGetter.h
/// @brief base class to get PropSharedLock instance
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-06-25
///

#ifndef SF1R_PROP_SHARED_LOCK_GETTER_H
#define SF1R_PROP_SHARED_LOCK_GETTER_H

#include "../faceted-submanager/faceted_types.h"
#include "PropSharedLock.h"

NS_FACETED_BEGIN

class PropSharedLockGetter
{
public:
    virtual ~PropSharedLockGetter() {}

    virtual const PropSharedLock* getSharedLock() { return NULL; }
};

NS_FACETED_END

#endif // SF1R_PROP_SHARED_LOCK_GETTER_H
