/**
 * @file QueryIntentFactory.h
 * @brief 
 * @author Kevin Lin
 * @date Created 2013-06-05
 */

#ifndef SF1R_QUERY_INTENT_FACTORY_H
#define SF1R_QUERY_INTENT_FACTORY_H

#include "QueryIntent.h"

namespace sf1r
{

class QueryIntentContext
{
};

class QueryIntentFactory
{
public:
    QueryIntent* createQueryIntent(QueryIntentContext& context);
};

}
#endif //QueryIntentFactory.h
