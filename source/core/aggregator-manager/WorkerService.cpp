#include "WorkerService.h"
#include <bundles/index/IndexSearchService.h>

namespace sf1r
{

bool WorkerService::processGetSearchResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    cout << "#[WorkerService::processGetSearchResult] " << actionItem.collectionName_ << endl;

    std::vector<std::vector<izenelib::util::UString> > propertyQueryTermList;

    if (!indexSearchService_->processSearchAction(
            const_cast<KeywordSearchActionItem&>(actionItem), resultItem, propertyQueryTermList))
    {
        return false;
    }

    resultItem.propertyQueryTermList_ = propertyQueryTermList;

    return true;
}

bool WorkerService::processGetSummaryResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    cout << "#[WorkerService::processGetSummaryResult] " << actionItem.collectionName_ << endl;

    if (!indexSearchService_->getSummaryMiningResult(
            const_cast<KeywordSearchActionItem&>(actionItem), resultItem, resultItem.propertyQueryTermList_))
    {
        return false;
    }

    return true;
}


}
