#include "LocalItemFactory.h"
#include "LocalItemIdGenerator.h"
#include "LocalItemManager.h"
#include <bundles/index/IndexSearchService.h>
#include <aggregator-manager/SearchWorker.h>
#include <ir/id_manager/IDManager.h>

namespace sf1r
{

LocalItemFactory::LocalItemFactory(IndexSearchService& indexSearchService)
    : indexSearchService_(indexSearchService)
{
}

ItemIdGenerator* LocalItemFactory::createItemIdGenerator()
{
    izenelib::ir::idmanager::IDManager* idManager = indexSearchService_.searchWorker_->idManager_.get();

    return new LocalItemIdGenerator(*idManager);
}

ItemManager* LocalItemFactory::createItemManager()
{
    DocumentManager* docManager = indexSearchService_.searchWorker_->documentManager_.get();

    return new LocalItemManager(*docManager);
}

} // namespace sf1r
