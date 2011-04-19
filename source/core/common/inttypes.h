#ifndef SF1V5_COMMON_INTTYPES_H
#define SF1V5_COMMON_INTTYPES_H
/**
 * @file core/common/inttypes.h
 * @author Ian Yang
 * @date Created <2010-03-19 12:42:56>
 */

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

namespace sf1r {

typedef uint32_t termid_t;
typedef uint32_t docid_t;
typedef uint32_t propertyid_t;
typedef uint32_t labelid_t;
typedef uint32_t loc_t;
typedef uint32_t count_t;
typedef uint32_t collectionid_t;

} // namespace sf1r

#endif // SF1V5_COMMON_INTTYPES_H
