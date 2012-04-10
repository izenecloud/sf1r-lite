#include "UpdateRecommendMaster.h"
#include <node-manager/RecommendMasterManager.h>

#include <memory> // for auto_ptr

namespace sf1r
{

UpdateRecommendMaster::UpdateRecommendMaster(
    const std::string& collection,
    UpdateRecommendWorker* localWorker
)
    : merger_(new UpdateRecommendMerger)
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
}

void UpdateRecommendMaster::updatePurchaseCoVisitMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    aggregator_->distributeRequest(collection_, "updatePurchaseCoVisitMatrix", oldItems, newItems, result);
}

void UpdateRecommendMaster::buildPurchaseSimMatrix(bool& result)
{
    aggregator_->distributeRequest(collection_, "buildPurchaseSimMatrix", result);
}

void UpdateRecommendMaster::updateVisitMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    aggregator_->distributeRequest(collection_, "updateVisitMatrix", oldItems, newItems, result);
}

void UpdateRecommendMaster::flushRecommendMatrix(bool& result)
{
    aggregator_->distributeRequest(collection_, "flushRecommendMatrix", result);
}

} // namespace sf1r
