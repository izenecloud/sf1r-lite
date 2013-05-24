/**
 * @file sf1r/search-manager/QueryPruneFactory.h
 * @author Hongliang Zhao
 * @date Created <2013-05-23>
 * @brief the factory class for QueryRefine
 */

#ifndef QUERY_REFINE_FACTORY_H
#define QUERY_REFINE_FACTORY_H

#include "QueryPruneBase.h"
#include "QueryPruneALL.h"
#include "common/type_defs.h"
#include <map>

namespace sf1r
{

enum QueryPruneType 
{

    QA_TRIGGER = 0,
    AND_TRIGGER
};

class QueryPruneFactory
{
public:
    QueryPruneFactory();
    ~QueryPruneFactory();

    QueryPruneBase* getQueryPrune(QueryPruneType type) const;

private:
    std::map<QueryPruneType, QueryPruneBase*> QueryPruneMap_;
};

}


#endif // QUERY_REFINE_FACTORY_H