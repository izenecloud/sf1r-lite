#ifndef CORE_SEARCH_MANAGER_SEARCH_MANAGER_POSTPROCESSOR_H
#define CORE_SEARCH_MANAGER_SEARCH_MANAGER_POSTPROCESSOR_H

#include <types.h>
#include <query-manager/ActionItem.h>
#include <common/ResultType.h>

namespace sf1r
{

class ProductRankerFactory;

class SearchManagerPostProcessor
{
    friend class SearchManager;
private:
    DISALLOW_COPY_AND_ASSIGN(SearchManagerPostProcessor);
    SearchManagerPostProcessor();
    ~SearchManagerPostProcessor();

    bool rerank(
            const KeywordSearchActionItem& actionItem, 
            KeywordSearchResult& resultItem);

    ProductRankerFactory* productRankerFactory_;
};

} // end of sf1r

#endif
