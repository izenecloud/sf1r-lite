#include "IndexWorkerController.h"
#include <process/controllers/CollectionHandler.h>
#include <bundles/index/IndexTaskService.h>

namespace sf1r
{

bool IndexWorkerController::checkWorker(std::string& error)
{
    indexWorker_ = collectionHandler_->indexTaskService_->indexWorker_;

    if (indexWorker_)
        return true;

    error = "IndexWorker not found";
    return false;
}

} // namespace sf1r
