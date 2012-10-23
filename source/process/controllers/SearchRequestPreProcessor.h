/**
 * @file SearchRequestPreProcessor.h
 * @brief base class to preprocess search request
 * @author Jun Jiang
 * @date Created 2012-10-22
 */

#ifndef SF1R_SEARCH_REQUEST_PRE_PROCESSOR_H
#define SF1R_SEARCH_REQUEST_PRE_PROCESSOR_H

#include <query-manager/ActionItem.h> // KeywordSearchActionItem
#include <common/ResultType.h> // KeywordSearchResult

namespace sf1r
{

class SearchRequestPreProcessor
{
public:
    virtual ~SearchRequestPreProcessor() {}

    virtual void process(
        KeywordSearchActionItem& actionItem,
        KeywordSearchResult& searchResult) = 0;
};

} // namespace sf1r

#endif // SF1R_SEARCH_REQUEST_PRE_PROCESSOR_H
