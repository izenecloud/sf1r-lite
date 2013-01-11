#include "IndexMerger.h"

namespace sf1r
{

void IndexMerger::index(const net::aggregator::WorkerResults<bool>& workerResults, bool& mergeResult)
{
    mergeResult = true;

    size_t workerNum = workerResults.size();
    for (size_t i = 0; i < workerNum; ++i)
    {
        if (!workerResults.result(i))
        {
            mergeResult = false;
            return;
        }
    }
}

void IndexMerger::HookDistributeRequest(const net::aggregator::WorkerResults<bool>& workerResults, bool& mergeResult)
{
    mergeResult = true;
    size_t workerNum = workerResults.size();
    for (size_t i = 0; i < workerNum; ++i)
    {
        if (!workerResults.result(i))
        {
            mergeResult = false;
            return;
        }
    }
}

} // namespace sf1r
