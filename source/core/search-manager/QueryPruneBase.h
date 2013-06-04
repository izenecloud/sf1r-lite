/**
 * @file sf1r/search-manager/QueryPruneBase.h
 * @author Hongliang Zhao
 * @date Created <2013-05-23>
 * @brief the base class for QueryPrune
 */

#ifndef QUERY_PRUNE_BASE_H
#define QUERY_PRUNE_BASE_H

#include <string>
#include <vector>
#include <util/ustring/UString.h>
namespace sf1r{

class QueryPruneBase
{
public:

    virtual ~QueryPruneBase() {}

    virtual bool queryPrune(std::string& query_orig,
                    std::vector<izenelib::util::UString> keywords,
                    std::string& newQuery) = 0;
};

}



 #endif // QUERY_PRUNE_BASE_H