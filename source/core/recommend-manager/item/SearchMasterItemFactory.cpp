#include "SearchMasterItemFactory.h"
#include "RemoteItemIdGenerator.h"
#include "SearchMasterItemManager.h"

namespace sf1r
{

SearchMasterItemFactory::SearchMasterItemFactory(
    const std::string& collection,
    IndexSearchService& indexSearchService
)
    : collection_(collection)
    , indexSearchService_(indexSearchService)
{
}

ItemIdGenerator* SearchMasterItemFactory::createItemIdGenerator()
{
    return new RemoteItemIdGenerator(collection_);
}

ItemManager* SearchMasterItemFactory::createItemManager()
{
    return new SearchMasterItemManager(collection_,
                                       createItemIdGenerator(),
                                       indexSearchService_);
}

} // namespace sf1r
