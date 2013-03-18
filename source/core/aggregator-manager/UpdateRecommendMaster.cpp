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

void UpdateRecommendMaster::HookDistributeRequestForUpdateRec()
{
    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI)
    {
        // from api do not need hook, just process as usually.
        return;
    }
    const std::string& reqdata = DistributeRequestHooker::get()->getAdditionData();
    bool ret = false;
    if (hooktype == Request::FromDistribute)
    {
        aggregator_->distributeRequest(collection_, "HookDistributeRequestForUpdateRec", (int)hooktype, reqdata, ret);
    }
    else
    {
        // local hook has been moved to the request controller.
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
    HookDistributeRequestForUpdateRec();

    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI || hooktype == Request::FromDistribute)
        aggregator_->distributeRequest(collection_, "updatePurchaseMatrix", oldItems, newItems, result);
    else
    {
        if (localWorker_)
            localWorker_->updatePurchaseMatrix(oldItems, newItems, result);
        else
            LOG(ERROR) << "no localWorker while do on local in " << __FUNCTION__;
    }
    printError(result, "updatePurchaseMatrix");
}

void UpdateRecommendMaster::updatePurchaseCoVisitMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    HookDistributeRequestForUpdateRec();

    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI || hooktype == Request::FromDistribute)
        aggregator_->distributeRequest(collection_, "updatePurchaseCoVisitMatrix", oldItems, newItems, result);
    else
    {
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
    HookDistributeRequestForUpdateRec();

    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI || hooktype == Request::FromDistribute)
        aggregator_->distributeRequest(collection_, "buildPurchaseSimMatrix", result);
    else
    {
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
    HookDistributeRequestForUpdateRec();

    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI || hooktype == Request::FromDistribute)
        aggregator_->distributeRequest(collection_, "updateVisitMatrix", oldItems, newItems, result);
    else
    {
        if (localWorker_)
            localWorker_->updateVisitMatrix(oldItems, newItems, result);
        else
            LOG(ERROR) << "no localWorker while do on local in " << __FUNCTION__;
    }
    printError(result, "updateVisitMatrix");
}

void UpdateRecommendMaster::flushRecommendMatrix(bool& result)
{
    HookDistributeRequestForUpdateRec();

    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI || hooktype == Request::FromDistribute)
        aggregator_->distributeRequest(collection_, "flushRecommendMatrix", result);
    else
    {
        if (localWorker_)
            localWorker_->flushRecommendMatrix(result);
        else
            LOG(ERROR) << "no localWorker while do on local in " << __FUNCTION__;
    }
    printError(result, "flushRecommendMatrix");
}

} // namespace sf1r
