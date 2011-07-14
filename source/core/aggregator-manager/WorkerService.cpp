#include "WorkerService.h"
#include <bundles/index/IndexSearchService.h>

namespace sf1r
{

bool WorkerService::processGetSearchResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    cout << "#[WorkerService] received request from: " << actionItem.collectionName_ << endl;

    std::vector<std::vector<izenelib::util::UString> > propertyQueryTermList;

    if (!indexSearchService_->processSearchAction(
            const_cast<KeywordSearchActionItem&>(actionItem), resultItem, propertyQueryTermList))
    {
        return false;
    }

    // todo, get by a second request
    if (!indexSearchService_->getSummaryMiningResult(
            const_cast<KeywordSearchActionItem&>(actionItem), resultItem, propertyQueryTermList))
    {
        return false;
    }

    return true;
}

}
