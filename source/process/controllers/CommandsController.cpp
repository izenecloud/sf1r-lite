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
#include <bundles/mining/MiningSearchService.h>
#include <core/mining-manager/MiningManager.h>
#include <bundles/recommend/RecommendTaskService.h>
#include <aggregator-manager/SearchAggregator.h>
#include <mining-manager/MiningQueryLogHandler.h>
#include <common/Keys.h>

namespace sf1r
{

using driver::Keys;

using namespace izenelib::driver;
using namespace izenelib::osgi;

CommandsController::CommandsController()
    : Sf1Controller(false)
{
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
    IZENELIB_DRIVER_BEFORE_HOOK(checkCollectionName());

    indexSearch_();
    indexRecommend_();
}

void CommandsController::indexSearch_()
{
    IndexTaskService* taskService = collectionHandler_->indexTaskService_;
    if (taskService)
    {
        if (request().callType() != Request::FromAPI)
        {
            DistributeRequestHooker::get()->setChainStatus(DistributeRequestHooker::ChainBegin);
        }
        // 0 indicates no limit
        Value::UintType documentCount = asUint(request()[Keys::document_count]);

        taskService->index(documentCount);
    }
}

void CommandsController::indexRecommend_()
{
    RecommendTaskService* taskService = collectionHandler_->recommendTaskService_;
    if (taskService)
    {
        if (request().callType() == Request::FromLog)
        {
            DistributeRequestHooker::get()->setChainStatus(DistributeRequestHooker::ChainEnd);
            taskService->buildCollection();
        }
        else
        {
            if (request().callType() != Request::FromAPI)
            {
                JobScheduler::get()->waitCurrentFinish(collectionName_);
                DistributeRequestHooker::get()->setChainStatus(DistributeRequestHooker::ChainEnd);
            }
            task_type task = boost::bind(&RecommendTaskService::buildCollection, taskService);
            JobScheduler::get()->addTask(task, collectionName_);
        }
    }
    else
    {
        DistributeRequestHooker::get()->processEndChain(false);
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
    IZENELIB_DRIVER_BEFORE_HOOK(checkCollectionName());

    IndexTaskService* taskService = collectionHandler_->indexTaskService_;
    if (!taskService)
    {
        response().addError("Request failed, index task service not found");
        return;
    }

    task_type task = boost::bind(&IndexTaskService::optimizeIndex, taskService);
    JobScheduler::get()->addTask(task, collectionName_);
}

} // namespace sf1r

