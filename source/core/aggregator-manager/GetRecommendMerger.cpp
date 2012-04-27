#include "GetRecommendMerger.h"
#include "SequenceMerger.h"

#include <iterator> // back_inserter

namespace sf1r
{

bool itemGreaterThan(
    const idmlib::recommender::RecommendItem& item1,
    const idmlib::recommender::RecommendItem& item2
)
{
    return item1.weight_ > item2.weight_;
}

void GetRecommendMerger::getCandidateSet(
    const net::aggregator::WorkerResults<CandidateSet>& workerResults,
    CandidateSet& mergeResult
)
{
    mergeCandidateSet_(workerResults, mergeResult);
}

void GetRecommendMerger::recommendFromCandidateSet(
    const net::aggregator::WorkerResults<RecommendItemVec>& workerResults,
    RecommendItemVec& mergeResult
)
{
    mergeRecommendResult_(workerResults, mergeResult);
}

void GetRecommendMerger::getCandidateSetFromWeight(
    const net::aggregator::WorkerResults<CandidateSet>& workerResults,
    CandidateSet& mergeResult
)
{
    mergeCandidateSet_(workerResults, mergeResult);
}

void GetRecommendMerger::recommendFromCandidateSetWeight(
    const net::aggregator::WorkerResults<RecommendItemVec>& workerResults,
    RecommendItemVec& mergeResult
)
{
    mergeRecommendResult_(workerResults, mergeResult);
}

void GetRecommendMerger::mergeRecommendResult_(
    const net::aggregator::WorkerResults<RecommendItemVec>& workerResults,
    RecommendItemVec& mergeResult
)
{
    typedef idmlib::recommender::RecommendItemVec::const_iterator ItemIter;
    SequenceMerger<ItemIter> merger;

    std::size_t workerNum = workerResults.size();
    for (std::size_t i=0; i<workerNum; ++i)
    {
        const RecommendItemVec& itemVec = workerResults.result(i);
        merger.addSequence(itemVec.begin(), itemVec.end());
    }

    merger.merge(std::back_inserter(mergeResult), itemGreaterThan);
}

void GetRecommendMerger::mergeCandidateSet_(
    const net::aggregator::WorkerResults<CandidateSet>& workerResults,
    CandidateSet& mergeResult
)
{
    std::size_t workerNum = workerResults.size();
    for (std::size_t i=0; i<workerNum; ++i)
    {
        const CandidateSet& candidateSet = workerResults.result(i);
        for (CandidateSet::const_iterator it = candidateSet.begin();
            it != candidateSet.end(); ++it)
        {
            mergeResult.insert(*it);
        }
    }
}

} // namespace sf1r
