/**
 * @file process/controllers/CommandsController.cpp
 * @author Ian Yang
 * @date Created <2010-06-01 10:26:16>
 */
#include "CommandsController.h"
#include "CollectionHandler.h"
#include <common/JobScheduler.h>
#include <process/common/XmlConfigParser.h>
#include <process/common/CollectionManager.h>
#include <bundles/index/IndexTaskService.h>
#include <bundles/index/IndexSearchService.h>
#include <bundles/recommend/RecommendTaskService.h>
#include <aggregator-manager/SearchAggregator.h>
#include <mining-manager/MiningQueryLogHandler.h>
#include <common/Keys.h>

namespace sf1r
{

using driver::Keys;

using namespace izenelib::driver;
using namespace izenelib::osgi;

bool CommandsController::parseCollection_()
{
    collection_ = asString(request()[Keys::collection]);
    if (collection_.empty())
    {
        response().addError("Require field collection in request.");
        return false;
    }

    return true;
}

/**
 * @brief Action \b index. Sends command "index" to SIA to ask it start index
 * SCD files.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b document_count (@c Uint = 0): For developer. Index at most \a document_count
 *   documents in each SCD files. 0 indicates no limit.
 *
 * @section response
 *
 * No extra fields.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "document_count": 1000
 * }
 * @endcode
 */
void CommandsController::index()
{
    IZENELIB_DRIVER_BEFORE_HOOK(parseCollection_());

    if (!SF1Config::get()->checkCollectionExist(collection_))
    {
        response().addError("Request failed, collection not found.");
        return;
    }

    if (!indexSearch_())
        return;

    indexRecommend_();
}

bool CommandsController::indexSearch_()
{
    // 0 indicates no limit
    Value::UintType documentCount = asUint(request()[Keys::document_count]);

    if (!collectionHandler_->indexTaskService_)
    {
        response().addError("Request failed, index service not found");
        return false;
    }

    return collectionHandler_->indexTaskService_->index(documentCount);
}

void CommandsController::indexRecommend_()
{
    std::string bundleName = "RecommendBundle-" + collection_;
    RecommendTaskService* taskService = static_cast<RecommendTaskService*>(
                                        CollectionManager::get()->getOSGILauncher().getService(bundleName, "RecommendTaskService"));
    if (taskService)
    {
        task_type task = boost::bind(&RecommendTaskService::buildCollection, taskService);
        JobScheduler::get()->addTask(task, collection_);
    }
}

/**
 * @brief Action \b index_query_log. Sends command "index_query_log" to rebuild all query related features.
 *
 * @section request
 *
 *
 * @section response
 *
 * No extra fields.
 *
 **/
void CommandsController::index_query_log()
{
    MiningQueryLogHandler::getInstance()->runEvents();
}

/**
 * @brief Action \b optimize_index. Sends command "optimize_index" to SIA to ask
 * it optimize the index structure.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * No extra fields.
 *
 * @section example
 * Request
 * @code
 * {
 *   "collection": "ChnWiki"
 * }
 * @endcode
 */
void CommandsController::optimize_index()
{
    IZENELIB_DRIVER_BEFORE_HOOK(parseCollection_());

    if (!SF1Config::get()->checkCollectionExist(collection_))
    {
        response().addError("Request failed, collection not found.");
        return;
    }
    std::string bundleName = "IndexBundle-" + collection_;
    IndexTaskService* indexService = static_cast<IndexTaskService*>(
                                         CollectionManager::get()->getOSGILauncher().getService(bundleName, "IndexTaskService"));
    if (!indexService)
    {
        response().addError("Request failed, index service not found");
        return;
    }
    task_type task = boost::bind(&IndexTaskService::optimizeIndex, indexService);
    JobScheduler::get()->addTask(task, collection_);
}

} // namespace sf1r

