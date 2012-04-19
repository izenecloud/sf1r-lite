#include "UpdateRecommendMaster.h"
#include <node-manager/RecommendMasterManager.h>
#include <node-manager/sharding/RecommendShardStrategy.h>

#include <glog/logging.h>
#include <memory> // for auto_ptr

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
        new UpdateRecommendAggregator(mergerProxy.get(), localWorkerProxy.get(), collection));

    mergerProxy.release();
    localWorkerProxy.release();

    RecommendMasterManager::get()->registerAggregator(aggregator_);
}

void UpdateRecommendMaster::updatePurchaseMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    aggregator_->distributeRequest(collection_, "updatePurchaseMatrix", oldItems, newItems, result);
    printError(result, "updatePurchaseMatrix");
}

void UpdateRecommendMaster::updatePurchaseCoVisitMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    aggregator_->distributeRequest(collection_, "updatePurchaseCoVisitMatrix", oldItems, newItems, result);
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
    aggregator_->distributeRequest(collection_, "buildPurchaseSimMatrix", result);
    printError(result, "buildPurchaseSimMatrix");
}

void UpdateRecommendMaster::updateVisitMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    aggregator_->distributeRequest(collection_, "updateVisitMatrix", oldItems, newItems, result);
    printError(result, "updateVisitMatrix");
}

void UpdateRecommendMaster::flushRecommendMatrix(bool& result)
{
    aggregator_->distributeRequest(collection_, "flushRecommendMatrix", result);
    printError(result, "flushRecommendMatrix");
}

} // namespace sf1r
