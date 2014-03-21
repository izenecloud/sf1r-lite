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
#include <bundles/mining/MiningTaskService.h>
#include <core/mining-manager/MiningManager.h>
#include <aggregator-manager/SearchAggregator.h>
#include <mining-manager/ad-index-manager/AdClickPredictor.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/MasterManagerBase.h>
#include <node-manager/NodeManagerBase.h>
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
}

void CommandsController::indexSearch_()
{
    IndexTaskService* taskService = collectionHandler_->indexTaskService_;
    if (taskService)
    {
        // 0 indicates no limit
        Value::UintType documentCount = asUint(request()[Keys::document_count]);

        int disable_sharding = 0;
        disable_sharding = asInt(request()[Keys::disable_sharding]);
        if(!taskService->index(documentCount,
                asString(request()[Keys::index_scd_path]),
                disable_sharding))
        {
            response().addError("index in search failed.");
        }
        else
        {
            if (MasterManagerBase::get()->isDistributed())
            {
                // push recommend request to queue
                if (request().callType() == Request::FromDistribute)
                {
                    std::string json_req = "{\"collection\":\"" + collectionName_ + "\",\"header\":{\"acl_tokens\":\"\",\"action\":\"index_recommend\",\"controller\":\"commands\"},\"uri\":\"commands/index_recommend\"}";
                    MasterManagerBase::get()->pushWriteReq(json_req);
                    LOG(INFO) << "a json_req pushed from " << __FUNCTION__ << ", data:" << json_req;
                }
                else
                {
                    LOG(INFO) << "not primary no need to send recommend";
                }
            }
        }
    }
}

void CommandsController::mining()
{
    IZENELIB_DRIVER_BEFORE_HOOK(checkCollectionName());
    MiningTaskService* miningService = collectionHandler_->miningTaskService_;
    if (!miningService)
    {
        response().addError("Request failed, mining service not found");
        return;
    }

    if (request().callType() != Request::FromAPI)
    {
        if( !miningService->DoMiningCollectionFromAPI() )
            response().addError("failed to do mining.");
        return;
    }
    task_type task = boost::bind(&MiningTaskService::DoMiningCollectionFromAPI, miningService);
    JobScheduler::get()->addTask(task, collectionName_);
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

    if (request().callType() != Request::FromAPI)
    {
        if( !taskService->optimizeIndex() )
            response().addError("failed to optimize index.");
        return;
    }
    task_type task = boost::bind(&IndexTaskService::optimizeIndex, taskService);
    JobScheduler::get()->addTask(task, collectionName_);
}

/*
 * @brief training ad click predictor
 */
void CommandsController::train_ctr_model()
{
    AdClickPredictor* adClickPredictor = AdClickPredictor::get();

    //LOG(INFO)<<"training ctr model" <<endl;
    if (!adClickPredictor->preProcess())
    {
        response().addError("request failed, preProcess failed");
        return;
    }
    if (!adClickPredictor->train())
    {
        response().addError("request failed when training");
        return;
    }
    if (!adClickPredictor->postProcess())
    {
        response().addError("request failed, postProcess failed");
    }
}

} // namespace sf1r
