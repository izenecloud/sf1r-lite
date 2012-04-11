#include "UpdateRecommendWorkerController.h"
#include <process/controllers/CollectionHandler.h>
#include <bundles/recommend/RecommendTaskService.h>

namespace sf1r
{

bool UpdateRecommendWorkerController::checkWorker(std::string& error)
{
    worker_ = collectionHandler_->recommendTaskService_->updateRecommendWorker();

    if (worker_)
        return true;

    error = "UpdateRecommendWorker not found";
    return false;
}

} // namespace sf1r
