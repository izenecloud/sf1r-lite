#include "SearchWorkerController.h"
#include <process/controllers/CollectionHandler.h>
#include <bundles/index/IndexSearchService.h>

namespace sf1r
{

bool SearchWorkerController::checkWorker(std::string& error)
{
    searchWorker_ = collectionHandler_->indexSearchService_->searchWorker_;

    if (searchWorker_)
        return true;

    error = "SearchWorker not found";
    return false;
}

} // namespace sf1r
