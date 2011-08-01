///
/// @file faceted-types.h
/// @brief types define for faceted
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-03
///

#ifndef SF1R_FACETED_TYPES_H_
#define SF1R_FACETED_TYPES_H_

#include <common/inttypes.h>
#include <util/ustring/UString.h>

#define NS_FACETED_BEGIN namespace sf1r{ namespace faceted {

#define NS_FACETED_END }}

NS_FACETED_BEGIN

typedef uint32_t CategoryIdType;
typedef izenelib::util::UString CategoryNameType;

NS_FACETED_END
#endif /* SF1R_FACETED_TYPES_H_ */
