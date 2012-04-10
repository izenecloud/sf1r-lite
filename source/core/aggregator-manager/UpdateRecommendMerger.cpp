#include "UpdateRecommendMerger.h"

namespace sf1r
{

void UpdateRecommendMerger::updatePurchaseMatrix(
    const net::aggregator::WorkerResults<bool>& workerResults,
    bool& mergeResult
)
{
    mergeUpdateResult_(workerResults, mergeResult);
}

void UpdateRecommendMerger::updatePurchaseCoVisitMatrix(
    const net::aggregator::WorkerResults<bool>& workerResults,
    bool& mergeResult
)
{
    mergeUpdateResult_(workerResults, mergeResult);
}

void UpdateRecommendMerger::buildPurchaseSimMatrix(
    const net::aggregator::WorkerResults<bool>& workerResults,
    bool& mergeResult
)
{
    mergeUpdateResult_(workerResults, mergeResult);
}

void UpdateRecommendMerger::updateVisitMatrix(
    const net::aggregator::WorkerResults<bool>& workerResults,
    bool& mergeResult
)
{
    mergeUpdateResult_(workerResults, mergeResult);
}

void UpdateRecommendMerger::flushRecommendMatrix(
    const net::aggregator::WorkerResults<bool>& workerResults,
    bool& mergeResult
)
{
    mergeUpdateResult_(workerResults, mergeResult);
}

void UpdateRecommendMerger::mergeUpdateResult_(
    const net::aggregator::WorkerResults<bool>& workerResults,
    bool& mergeResult
)
{
    mergeResult = true;

    std::size_t workerNum = workerResults.size();
    for (std::size_t i = 0; i < workerNum; ++i)
    {
        if (!workerResults.result(i))
        {
            mergeResult = false;
            return;
        }
    }
}

} // namespace sf1r
