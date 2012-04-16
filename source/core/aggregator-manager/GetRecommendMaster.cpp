#include "GetRecommendMaster.h"
#include <node-manager/RecommendMasterManager.h>
#include <node-manager/sharding/RecommendShardStrategy.h>

#include <memory> // for auto_ptr
#include <set>

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
    RecommendShardStrategy* shardStrategy,
    GetRecommendWorker* localWorker
)
    : collection_(collection)
    , merger_(new GetRecommendMerger)
    , shardStrategy_(shardStrategy)
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
    std::set<itemid_t> candidateSet;
    aggregator_->distributeRequest(collection_, "getCandidateSet",
                                   inputParam.inputItemIds, candidateSet);

    if (candidateSet.empty())
        return;

    aggregator_->distributeRequest(collection_, "recommendFromCandidateSet",
                                   inputParam, candidateSet, results);

    limitSize(results, inputParam.limit);
}

void GetRecommendMaster::recommendPurchaseFromWeight(
    const RecommendInputParam& inputParam,
    idmlib::recommender::RecommendItemVec& results
)
{
    std::set<itemid_t> candidateSet;
    aggregator_->distributeRequest(collection_, "getCandidateSetFromWeight",
                                   inputParam.itemWeightMap, candidateSet);

    if (candidateSet.empty())
        return;

    aggregator_->distributeRequest(collection_, "recommendFromCandidateSetWeight",
                                   inputParam, candidateSet, results);

    limitSize(results, inputParam.limit);
}

void GetRecommendMaster::recommendVisit(
    const RecommendInputParam& inputParam,
    idmlib::recommender::RecommendItemVec& results
)
{
    shardid_t workerId = shardStrategy_->getShardId(inputParam.inputItemIds[0]);
    aggregator_->singleRequest(collection_, "recommendVisit", inputParam, results, workerId);

    limitSize(results, inputParam.limit);
}

} // namespace sf1r
