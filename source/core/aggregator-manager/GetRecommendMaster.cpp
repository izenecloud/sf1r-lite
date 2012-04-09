#include "GetRecommendMaster.h"
#include <node-manager/RecommendMasterManager.h>

#include <glog/logging.h>
#include <memory> // for auto_ptr

namespace
{

template <class ContainerType>
void limitSize(ContainerType& container, std::size_t limit)
{
    if (container.size() > limit)
    {
        container.erase(container.begin() + limit,
                        container.end());
    }
}

}

namespace sf1r
{

GetRecommendMaster::GetRecommendMaster(
    const std::string& collection,
    GetRecommendWorker* localWorker
)
    : merger_(new GetRecommendMerger)
{
    std::auto_ptr<GetRecommendMergerProxy> mergerProxy(new GetRecommendMergerProxy(merger_.get()));
    merger_->bindCallProxy(*mergerProxy);

    std::auto_ptr<GetRecommendWorkerProxy> localWorkerProxy;
    if (localWorker)
    {
        localWorkerProxy.reset(new GetRecommendWorkerProxy(localWorker));
        localWorker->bindCallProxy(*localWorkerProxy);
    }

    aggregator_.reset(
        new GetRecommendAggregator(mergerProxy.get(), localWorkerProxy.get(), collection));

    mergerProxy.release();
    localWorkerProxy.release();

    RecommendMasterManager::get()->registerAggregator(aggregator_);
}

void GetRecommendMaster::recommendPurchase(
    const RecommendInputParam& inputParam,
    idmlib::recommender::RecommendItemVec& results
)
{
    aggregator_->distributeRequest(collection_, "recommendPurchase", inputParam, results);
    limitSize(results, inputParam.limit);
}

void GetRecommendMaster::recommendPurchaseFromWeight(
    const RecommendInputParam& inputParam,
    idmlib::recommender::RecommendItemVec& results
)
{
    aggregator_->distributeRequest(collection_, "recommendPurchaseFromWeight", inputParam, results);
    limitSize(results, inputParam.limit);
}

void GetRecommendMaster::recommendVisit(
    const RecommendInputParam& inputParam,
    idmlib::recommender::RecommendItemVec& results
)
{
    aggregator_->distributeRequest(collection_, "recommendVisit", inputParam, results);
    limitSize(results, inputParam.limit);
}

} // namespace sf1r
