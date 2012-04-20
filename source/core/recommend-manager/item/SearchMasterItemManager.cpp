#include "SearchMasterItemManager.h"
#include "ItemIdGenerator.h"
#include <bundles/index/IndexSearchService.h>

#include <glog/logging.h>

namespace sf1r
{

SearchMasterItemManager::SearchMasterItemManager(
    const std::string& collection,
    ItemIdGenerator* itemIdGenerator,
    IndexSearchService& indexSearchService
)
    : RemoteItemManagerBase(collection, itemIdGenerator)
    , indexSearchService_(indexSearchService)
{
}

bool SearchMasterItemManager::sendRequest_(
    const GetDocumentsByIdsActionItem& request,
    RawTextResultFromSIA& response
)
{
    indexSearchService_.getDocumentsByIds(request, response);
    return true;
}

} // namespace sf1r
