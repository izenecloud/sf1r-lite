///
/// @File   QueryTypeDef.h
/// @brief  The file which stores query type list and its number definition.
/// @author Dohyun Yun
/// @date   2009.08.10
/// @details
/// - Log
///     

#ifndef _QUERYTYPEDEF_H_
#define _QUERYTYPEDEF_H_

#include <common/PropertyValue.h>

namespace sf1r {
namespace QueryFiltering {

enum FilteringOperation
{
    NULL_OPERATOR = 0,
    EQUAL,
    NOT_EQUAL,
    INCLUDE,
    EXCLUDE,
    GREATER_THAN,
    GREATER_THAN_EQUAL,
    LESS_THAN,
    LESS_THAN_EQUAL,
    RANGE,
    PREFIX,
    SUFFIX,
    SUB_STRING,
    MAX_OPERATOR
}; // end - enum

typedef std::pair<FilteringOperation , std::string> FilteringKeyType; 
typedef std::pair<FilteringKeyType , std::vector<PropertyValue> > FilteringType;

} // end namespace QueryFiltering
} // end namespace sf1r

#endif // _QUERYTYPEDEF_H_
