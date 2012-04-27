#include "UpdateRecommendWorkerController.h"
#include <process/controllers/CollectionHandler.h>

namespace sf1r
{

bool UpdateRecommendWorkerController::checkWorker(std::string& error)
{
    worker_ = collectionHandler_->updateRecommendWorker_;

    if (worker_)
        return true;

    error = "UpdateRecommendWorker not found";
    return false;
}

} // namespace sf1r
