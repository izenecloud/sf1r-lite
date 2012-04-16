#include "GetRecommendWorkerController.h"
#include <process/controllers/CollectionHandler.h>

namespace sf1r
{

bool GetRecommendWorkerController::checkWorker(std::string& error)
{
    worker_ = collectionHandler_->getRecommendWorker_;

    if (worker_)
        return true;

    error = "GetRecommendWorker not found";
    return false;
}

} // namespace sf1r
