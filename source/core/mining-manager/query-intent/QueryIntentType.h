/**
 * @file QueryIntentType.h
 * @author Kevin Lin
 * @date Created 2013-06-05
 */

#ifndef SF1R_QUERY_INTENT_TYPE_H
#define SF1R_QUERY_INTENT_TYPE_H

#include <cstddef>

namespace sf1r
{

typedef enum
{
    COMMODITY = 0,
    MERCHANT,
    CATEGORY
}QueryIntentType;

}

#endif

