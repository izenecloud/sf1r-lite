/**
 * @file IndexMerger.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief merge indexing results
 */
#ifndef SF1R_INDEX_MERGER_H_
#define SF1R_INDEX_MERGER_H_

#include <net/aggregator/BindCallProxyBase.h>
#include <net/aggregator/WorkerResults.h>

namespace sf1r
{

class IndexMerger : public net::aggregator::BindCallProxyBase<IndexMerger>
{
public:
    virtual bool bindCallProxy(CallProxyType& proxy)
    {
        BIND_CALL_PROXY_BEGIN(IndexMerger, proxy)
        BIND_CALL_PROXY_2(index, net::aggregator::WorkerResults<bool>, bool)
        BIND_CALL_PROXY_2(HookDistributeRequestForIndex, net::aggregator::WorkerResults<bool>, bool)
        BIND_CALL_PROXY_END()
    }

    void index(const net::aggregator::WorkerResults<bool>& workerResults, bool& mergeResult);
    void HookDistributeRequestForIndex(const net::aggregator::WorkerResults<bool>& workerResults, bool& mergeResult);
};


} // namespace sf1r

#endif /* SF1R_INDEX_MERGER_H_ */
