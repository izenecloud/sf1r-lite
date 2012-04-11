#include "GetRecommendWorkerController.h"
#include <process/controllers/CollectionHandler.h>
#include <bundles/recommend/RecommendSearchService.h>

namespace sf1r
{

bool GetRecommendWorkerController::checkWorker(std::string& error)
{
    worker_ = collectionHandler_->recommendSearchService_->getRecommendWorker();

    if (worker_)
        return true;

    error = "GetRecommendWorker not found";
    return false;
}

} // namespace sf1r
