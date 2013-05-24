/**
 * @file sf1r/search-manager/QueryPruneALL.h
 * @author Hongliang Zhao
 * @date Created <2013-05-23>
 * @brief include all query Prune algorithms
 */

#ifndef QUERY_PRUNE_ALL_H
#define QUERY_PRUNE_ALL_H 

#include "QueryPruneBase.h"
#include "QueryHelper.h"

namespace  sf1r
{

class AddQueryPrune : public QueryPruneBase
{
public:
    AddQueryPrune()
    {}

    ~AddQueryPrune()
    {}

    bool queryPrune(const std::vector<izenelib::util::UString> keywords, std::string& newQuery)
    {
        //care about muti thread....
        return true;
    }

};

class QAQueryPrune : public QueryPruneBase
{
public:
    QAQueryPrune()
    {
    }
    ~QAQueryPrune()
    {
    }
    bool queryPrune(const std::vector<izenelib::util::UString> keywords, std::string& newQuery)
    {
        assembleDisjunction(keywords, newQuery);
        return true;
    }

};

}
#endif //QUERY_Prune_ALL_H