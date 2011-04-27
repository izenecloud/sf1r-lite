/**
 * @file RecTypes.h
 * @author Jun Jiang
 * @date 2011-04-18
 */

#ifndef REC_TYPES_H
#define REC_TYPES_H

#ifndef SF1R_NO_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#else
# ifdef HAVE_STDINT_H
#  include <stdint.h>
# else
#  ifdef HAVE_SYS_TYPES_H
#   include <sys/types.h>
#  endif
#  ifdef HAVE_STDDEF_H
#   include <stddef.h>
#  endif
# endif
#endif

#include <ir/id_manager/IDGenerator.h>

#include <set>
#include <string>

namespace sf1r
{

typedef uint32_t userid_t;
typedef uint32_t itemid_t;

typedef std::set<itemid_t> ItemIdSet;
typedef izenelib::ir::idmanager::UniqueIDGenerator<std::string, uint32_t, ReadWriteLock> RecIdGenerator;
} // namespace sf1r

#endif // REC_TYPES_H
