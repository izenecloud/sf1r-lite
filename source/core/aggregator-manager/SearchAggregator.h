/**
 * @file SearchAggregator.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief index and minging search aggregator
 */
#ifndef SEARCH_AGGREGATOR_H_
#define SEARCH_AGGREGATOR_H_

#include <net/aggregator/Typedef.h>
#include <net/aggregator/Aggregator.h>
#include <net/aggregator/AggregatorConfig.h>

#include "SearchWorker.h"

using namespace net::aggregator;

namespace sf1r
{

class KeywordSearchResult;
class MiningManager;

typedef WorkerCaller<SearchWorker> SearchWorkerCaller;

class SearchAggregator : public Aggregator<SearchAggregator, SearchWorkerCaller>
{
public:
    SearchAggregator(SearchWorker* searchWorker)
        : TOP_K_NUM(4000)
    {
        localWorkerCaller_.reset(new SearchWorkerCaller(searchWorker));

        // xxx
        ADD_FUNC_TO_WORKER_CALLER(SearchWorkerCaller, localWorkerCaller_, SearchWorker, getDistSearchInfo);
        ADD_FUNC_TO_WORKER_CALLER(SearchWorkerCaller, localWorkerCaller_, SearchWorker, getDistSearchResult);
        ADD_FUNC_TO_WORKER_CALLER(SearchWorkerCaller, localWorkerCaller_, SearchWorker, getSummaryResult);
        ADD_FUNC_TO_WORKER_CALLER(SearchWorkerCaller, localWorkerCaller_, SearchWorker, getSummaryMiningResult);
        ADD_FUNC_TO_WORKER_CALLER(SearchWorkerCaller, localWorkerCaller_, SearchWorker, getDocumentsByIds);
        ADD_FUNC_TO_WORKER_CALLER(SearchWorkerCaller, localWorkerCaller_, SearchWorker, getInternalDocumentId);
        ADD_FUNC_TO_WORKER_CALLER(SearchWorkerCaller, localWorkerCaller_, SearchWorker, getSimilarDocIdList);
        ADD_FUNC_TO_WORKER_CALLER(SearchWorkerCaller, localWorkerCaller_, SearchWorker, clickGroupLabel);
        ADD_FUNC_TO_WORKER_CALLER(SearchWorkerCaller, localWorkerCaller_, SearchWorker, visitDoc);
    }

public:
    /**
     * TODO overload aggregate() to aggregate result(s) got from server node(s).
     *
     * @{
     */

    bool aggregate(const std::string& func,DistKeywordSearchInfo& result, const std::vector<std::pair<workerid_t, DistKeywordSearchInfo> >& resultList)
    {
        if (func == "getDistSearchInfo")
        {
            aggregateDistSearchInfo(result, resultList);
            return true;
        }
        return false;
    }


    bool aggregate(const std::string& func, DistKeywordSearchResult& result, const std::vector<std::pair<workerid_t, DistKeywordSearchResult> >& resultList)
    {
        if (func == "getDistSearchResult")
        {
            aggregateDistSearchResult(result, resultList);
            return true;
        }
        return false;
    }

    bool aggregate(const std::string& func, KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList)
    {
        if (func == "getSummaryResult")
        {
            aggregateSummaryResult(result, resultList);
            return true;
        }
        else if (func == "getSummaryMiningResult")
        {
            aggregateSummaryMiningResult(result, resultList);
            return true;
        }
        return false;
    }

    bool aggregate(const std::string& func, RawTextResultFromSIA& result, const std::vector<std::pair<workerid_t, RawTextResultFromSIA> >& resultList)
    {
        if (func == "getDocumentsByIds")
        {
            aggregateDocumentsResult(result, resultList);
            return true;
        }
        return false;
    }

    bool aggregate(const std::string& func, uint64_t& result, const std::vector<std::pair<workerid_t, uint64_t> >& resultList)
    {
        if (func == "getInternalDocumentId")
        {
            aggregateInternalDocumentId(result, resultList);
            return true;
        }
        return false;
    }

    bool aggregate(const std::string& func, bool& ret, const std::vector<std::pair<workerid_t, bool> >& resultList)
    {
        if (func == "clickGroupLabel")
        {
            // xxx
            ret = true;
            return true;
        }
        return false;
    }

    /** @}*/

private:
    /// search
    void aggregateDistSearchInfo(DistKeywordSearchInfo& result, const std::vector<std::pair<workerid_t, DistKeywordSearchInfo> >& resultList);

    void aggregateDistSearchResult(DistKeywordSearchResult& result, const std::vector<std::pair<workerid_t, DistKeywordSearchResult> >& resultList);

    void aggregateSummaryMiningResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList);

    /// documents
    void aggregateSummaryResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList);

    void aggregateDocumentsResult(RawTextResultFromSIA& result, const std::vector<std::pair<workerid_t, RawTextResultFromSIA> >& resultList);

    void aggregateInternalDocumentId(uint64_t& result, const std::vector<std::pair<workerid_t, uint64_t> >& resultList);

    /// mining
    void aggregateMiningResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList);

    //void aggregateSimilarDocIdList(SimilarDocIdListType& result, const std::vector<std::pair<workerid_t, SimilarDocIdListType> >& resultList);


public:
    /**
     * Split data by workerid to sub data for requesting different workers.
     * @param result [IN]
     * @param resultList [OUT]
     * @return true if successfully splited, or false.
     */
    bool splitSearchResultByWorkerid(const KeywordSearchResult& result, std::map<workerid_t, boost::shared_ptr<KeywordSearchResult> >& resultMap);

    /**
     *
     * @param actionItem [IN]
     * @param actionItemMap [OUT]
     * @return true if successfully splited, or false.
     */
    bool splitGetDocsActionItemByWorkerid(
            const GetDocumentsByIdsActionItem& actionItem,
            std::map<workerid_t, boost::shared_ptr<GetDocumentsByIdsActionItem> >& actionItemMap);

private:
    boost::shared_ptr<MiningManager> miningManager_;
    int TOP_K_NUM;

    friend class IndexSearchService;
    friend class IndexBundleActivator;
};


} // end - namespace

#endif /* SEARCH_AGGREGATOR_H_ */
