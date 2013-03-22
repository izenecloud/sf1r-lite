#include "UpdateRecommendMaster.h"
#include <node-manager/MasterManagerBase.h>
#include <node-manager/sharding/RecommendShardStrategy.h>
#include <node-manager/DistributeRequestHooker.h>
#include <util/driver/Request.h>

#include <glog/logging.h>
#include <memory> // for auto_ptr

using namespace izenelib::driver;

namespace
{

void printError(bool result, const std::string& funcName)
{
    if (!result)
    {
        LOG(ERROR) << "failed in recommend master function " << funcName;
    }
}

}

namespace sf1r
{

UpdateRecommendMaster::UpdateRecommendMaster(
    const std::string& collection,
    RecommendShardStrategy* shardStrategy,
    UpdateRecommendWorker* localWorker
)
    : collection_(collection)
    , merger_(new UpdateRecommendMerger)
    , localWorker_(localWorker)
    , shardStrategy_(shardStrategy)
{
    std::auto_ptr<UpdateRecommendMergerProxy> mergerProxy(new UpdateRecommendMergerProxy(merger_.get()));
    merger_->bindCallProxy(*mergerProxy);

    std::auto_ptr<UpdateRecommendWorkerProxy> localWorkerProxy;
    if (localWorker)
    {
        localWorkerProxy.reset(new UpdateRecommendWorkerProxy(localWorker));
        localWorker->bindCallProxy(*localWorkerProxy);
    }

    aggregator_.reset(
        new UpdateRecommendAggregator(mergerProxy.get(), localWorkerProxy.get(),
            Sf1rTopology::getServiceName(Sf1rTopology::RecommendService), collection));

    mergerProxy.release();
    localWorkerProxy.release();

    MasterManagerBase::get()->registerAggregator(aggregator_);
}

void UpdateRecommendMaster::HookDistributeCBRequestForUpdateRec(const std::string& callback_data, bool include_self)
{
    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI)
    {
        // from api do not need hook, just process as usually.
        return;
    }
    bool ret = false;
    if (hooktype == Request::FromDistribute)
    {
        //aggregator_->distributeRequestWithoutLocal(collection_, "HookDistributeRequestForUpdateRec", (int)hooktype,
        //    callback_data, ret);
    }
    else
    {
        ret = true;
    }
    if (!ret)
    {
        LOG(WARNING) << "Hook request for shard nodes failed." << __FUNCTION__;
    }
}

void UpdateRecommendMaster::updatePurchaseMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI)
        aggregator_->distributeRequest(collection_, "updatePurchaseMatrix", oldItems, newItems, result);
    else
    {
        if (hooktype == Request::FromDistribute)
        {
            UpdateRecCallbackReqLog reqlog;
            reqlog.req_json_data = collection_ + "_updatePurchaseMatrix";
            reqlog.oldItems = oldItems;
            reqlog.newItems = newItems;
            std::string packed_data;
            ReqLogMgr::packReqLogData(reqlog, packed_data);
            HookDistributeCBRequestForUpdateRec(packed_data, false);
        }
        if (localWorker_)
            localWorker_->updatePurchaseMatrix(oldItems, newItems, result);
        else
            LOG(INFO) << "no localWorker while do on local in " << __FUNCTION__;
    }
    printError(result, "updatePurchaseMatrix");
}

void UpdateRecommendMaster::updatePurchaseCoVisitMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI)
        aggregator_->distributeRequest(collection_, "updatePurchaseCoVisitMatrix", oldItems, newItems, result);
    else
    {
        if (hooktype == Request::FromDistribute)
        {
            UpdateRecCallbackReqLog reqlog;
            reqlog.req_json_data = collection_ + "_updatePurchaseCoVisitMatrix";
            reqlog.oldItems = oldItems;
            reqlog.newItems = newItems;
            std::string packed_data;
            ReqLogMgr::packReqLogData(reqlog, packed_data);
            HookDistributeCBRequestForUpdateRec(packed_data, false);
        }
        if (localWorker_)
            localWorker_->updatePurchaseCoVisitMatrix(oldItems, newItems, result);
        else
            LOG(ERROR) << "no localWorker while do on local in " << __FUNCTION__;
    }
    printError(result, "updatePurchaseCoVisitMatrix");
}

bool UpdateRecommendMaster::needRebuildPurchaseSimMatrix() const
{
    // when distributed on multiple nodes,
    // we need to rebuild similarity matrix periodically to keep it updated.
    return shardStrategy_->getShardNum() > 1;
}

void UpdateRecommendMaster::buildPurchaseSimMatrix(bool& result)
{
    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI)
        aggregator_->distributeRequest(collection_, "buildPurchaseSimMatrix", result);
    else
    {
        if (hooktype == Request::FromDistribute)
        {
            BuildPurchaseSimCallbackReqLog reqlog;
            reqlog.req_json_data = collection_ + "_buildPurchaseSimMatrix";
            std::string packed_data;
            ReqLogMgr::packReqLogData(reqlog, packed_data);
            HookDistributeCBRequestForUpdateRec(packed_data, false);
        }

        if (localWorker_)
            localWorker_->buildPurchaseSimMatrix(result);
        else
            LOG(ERROR) << "no localWorker while do on local in " << __FUNCTION__;
    }
    printError(result, "buildPurchaseSimMatrix");
}

void UpdateRecommendMaster::updateVisitMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI)
        aggregator_->distributeRequest(collection_, "updateVisitMatrix", oldItems, newItems, result);
    else
    {
        if (hooktype == Request::FromDistribute)
        {
            UpdateRecCallbackReqLog reqlog;
            reqlog.req_json_data = collection_ + "_updateVisitMatrix";
            reqlog.oldItems = oldItems;
            reqlog.newItems = newItems;
            std::string packed_data;
            ReqLogMgr::packReqLogData(reqlog, packed_data);
            HookDistributeCBRequestForUpdateRec(packed_data, false);
        }

        if (localWorker_)
            localWorker_->updateVisitMatrix(oldItems, newItems, result);
        else
            LOG(ERROR) << "no localWorker while do on local in " << __FUNCTION__;
    }
    printError(result, "updateVisitMatrix");
}

void UpdateRecommendMaster::flushRecommendMatrix(bool& result)
{
    aggregator_->distributeRequest(collection_, "flushRecommendMatrix", result);
    printError(result, "flushRecommendMatrix");
}

} // namespace sf1r
