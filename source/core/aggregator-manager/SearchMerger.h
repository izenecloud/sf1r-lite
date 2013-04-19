/**
 * @file SearchMerger.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief merge search and minging results
 */
#ifndef SF1R_SEARCH_MERGER_H_
#define SF1R_SEARCH_MERGER_H_

#include <net/aggregator/BindCallProxyBase.h>
#include <net/aggregator/WorkerResults.h>
#include <common/inttypes.h>

#include <map>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

class MiningManager;
class DistKeywordSearchInfo;
class KeywordSearchResult;
class RawTextResultFromSIA;
class GetDocumentsByIdsActionItem;

class SearchMerger : public net::aggregator::BindCallProxyBase<SearchMerger>
{
public:
    SearchMerger(std::size_t topKNum = 4000) : TOP_K_NUM(topKNum) {}

    virtual bool bindCallProxy(CallProxyType& proxy)
    {
        BIND_CALL_PROXY_BEGIN(SearchMerger, proxy)
        BIND_CALL_PROXY_2(getDistSearchInfo, net::aggregator::WorkerResults<DistKeywordSearchInfo>, DistKeywordSearchInfo)
        BIND_CALL_PROXY_2(getDistSearchResult, net::aggregator::WorkerResults<KeywordSearchResult>, KeywordSearchResult)
        BIND_CALL_PROXY_2(getSummaryResult, net::aggregator::WorkerResults<KeywordSearchResult>, KeywordSearchResult)
        BIND_CALL_PROXY_2(getSummaryMiningResult, net::aggregator::WorkerResults<KeywordSearchResult>, KeywordSearchResult)
        BIND_CALL_PROXY_2(getDocumentsByIds, net::aggregator::WorkerResults<RawTextResultFromSIA>, RawTextResultFromSIA)
        BIND_CALL_PROXY_2(getInternalDocumentId, net::aggregator::WorkerResults<uint64_t>, uint64_t)
        BIND_CALL_PROXY_2(clickGroupLabel, net::aggregator::WorkerResults<bool>, bool)
        BIND_CALL_PROXY_2(HookDistributeRequestForSearch, net::aggregator::WorkerResults<bool>, bool)
        BIND_CALL_PROXY_END()
    }

    void getDistSearchInfo(const net::aggregator::WorkerResults<DistKeywordSearchInfo>& workerResults, DistKeywordSearchInfo& mergeResult);
    void getDistSearchResult(const net::aggregator::WorkerResults<KeywordSearchResult>& workerResults, KeywordSearchResult& mergeResult);
    void getSummaryResult(const net::aggregator::WorkerResults<KeywordSearchResult>& workerResults, KeywordSearchResult& mergeResult);
    void getSummaryMiningResult(const net::aggregator::WorkerResults<KeywordSearchResult>& workerResults, KeywordSearchResult& mergeResult);
    void getMiningResult(const net::aggregator::WorkerResults<KeywordSearchResult>& workerResults, KeywordSearchResult& mergeResult);
    void getDocumentsByIds(const net::aggregator::WorkerResults<RawTextResultFromSIA>& workerResults, RawTextResultFromSIA& mergeResult);
    void getInternalDocumentId(const net::aggregator::WorkerResults<uint64_t>& workerResults, uint64_t& mergeResult);
    void clickGroupLabel(const net::aggregator::WorkerResults<bool>& workerResults, bool& mergeResult);

    /**
     * Split data by workerid to sub data for requesting different workers.
     * @param totalResult [IN]
     * @param resultList [OUT]
     */
    void splitSearchResultByWorkerid(const KeywordSearchResult& totalResult, std::map<workerid_t, KeywordSearchResult>& resultMap);

    /**
     *
     * @param actionItem [IN]
     * @param actionItemMap [OUT]
     * @return true if successfully splited, or false.
     */
    bool splitGetDocsActionItemByWorkerid(
        const GetDocumentsByIdsActionItem& actionItem,
        std::map<workerid_t, GetDocumentsByIdsActionItem>& actionItemMap);

    void HookDistributeRequestForSearch(const net::aggregator::WorkerResults<bool>& workerResults, bool& mergeResult);
private:
    std::size_t TOP_K_NUM;
    boost::shared_ptr<MiningManager> miningManager_;

    friend class IndexBundleActivator;
};


} // namespace sf1r

#endif /* SF1R_SEARCH_MERGER_H_ */
