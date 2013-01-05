/**
 * @file SearchThreadParam.h
 * @brief the search parameter used in one thread.
 * @author Vincent Lee, Jun Jiang
 * @date Created 2012-12-21
 */

#ifndef SF1R_SEARCH_THREAD_PARAM_H
#define SF1R_SEARCH_THREAD_PARAM_H

#include "CustomRanker.h"
#include <common/ResultType.h>
#include <mining-manager/group-manager/GroupRep.h>
#include <mining-manager/faceted-submanager/ontology_rep.h>
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>

namespace sf1r
{
class SearchKeywordOperation;
class Sorter;
class HitQueue;
class DistKeywordSearchInfo;

struct SearchThreadParam
{
    const SearchKeywordOperation* actionOperation;
    DistKeywordSearchInfo* distSearchInfo;

    std::size_t totalCount;
    sf1r::PropertyRange propertyRange;
    std::map<std::string, unsigned int> counterResults;

    faceted::GroupRep groupRep;
    faceted::OntologyRep attrRep;
    int originAttrGroupNum;

    boost::shared_ptr<Sorter> pSorter;
    CustomRankerPtr customRanker;

    std::size_t heapSize;
    boost::shared_ptr<HitQueue> scoreItemQueue;

    int runningNode;
    std::size_t threadId;
    std::size_t docIdBegin;
    std::size_t docIdEnd;

    bool isSuccess;

    SearchThreadParam(
        const SearchKeywordOperation* _actionOperation,
        DistKeywordSearchInfo* _distSearchInfo,
        std::size_t _heapSize,
        int  _runningNode)
        : actionOperation(_actionOperation)
        , distSearchInfo(_distSearchInfo)
        , totalCount(0)
        , originAttrGroupNum(0)
        , heapSize(_heapSize)
        , runningNode(_runningNode)
        , threadId(0)
        , docIdBegin(0)
        , docIdEnd(0)
        , isSuccess(false)
    {}
};

} // namespace sf1r

#endif // SF1R_SEARCH_THREAD_PARAM_H
