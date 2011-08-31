#include "WorkerService.h"
#include <bundles/index/IndexSearchService.h>

namespace sf1r
{

bool WorkerService::processGetSearchResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    cout << "#[WorkerService::processGetSearchResult] " << actionItem.collectionName_ << endl;



    return true;
}

bool WorkerService::processGetSummaryResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    cout << "#[WorkerService::processGetSummaryResult] " << actionItem.collectionName_ << endl;



    return true;
}


}
