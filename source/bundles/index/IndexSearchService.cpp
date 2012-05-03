#include "IndexSearchService.h"

#include <aggregator-manager/SearchMerger.h>
#include <aggregator-manager/SearchWorker.h>

#include <common/SearchCache.h>
#include <common/SFLogger.h>
#include <common/type_defs.h>

namespace sf1r
{

IndexSearchService::IndexSearchService(IndexBundleConfiguration* config)
    : bundleConfig_(config)
    , searchMerger_(NULL)
    , searchCache_(new SearchCache(bundleConfig_->masterSearchCacheNum_))
{
}

IndexSearchService::~IndexSearchService()
{
}

boost::shared_ptr<SearchAggregator> IndexSearchService::getSearchAggregator()
{
    return searchAggregator_;
}

const IndexBundleConfiguration* IndexSearchService::getBundleConfig()
{
    return bundleConfig_;
}

void IndexSearchService::OnUpdateSearchCache()
{
    searchCache_->clear();
}

bool IndexSearchService::getSearchResult(
    KeywordSearchActionItem& actionItem,
    KeywordSearchResult& resultItem
)
{
    CREATE_SCOPED_PROFILER (query, "IndexSearchService", "processGetSearchResults all: total query time");

    if (!bundleConfig_->isMasterAggregator())
    {
        bool ret = searchWorker_->doLocalSearch(actionItem, resultItem);
        net::aggregator::WorkerResults<KeywordSearchResult> workerResults;
        workerResults.add(0, resultItem);
        searchMerger_->getMiningResult(workerResults, resultItem);
        return ret;
    }

    /// Perform distributed search by aggregator
    DistKeywordSearchResult distResultItem;
    distResultItem.distSearchInfo_.effective_ = true;
    distResultItem.distSearchInfo_.nodeType_ = DistKeywordSearchInfo::NODE_WORKER;

#ifdef GATHER_DISTRIBUTED_SEARCH_INFO
    distResultItem.distSearchInfo_.option_ = DistKeywordSearchInfo::OPTION_GATHER_INFO;
    searchAggregator_->distributeRequest<KeywordSearchActionItem, DistKeywordSearchInfo>(
            actionItem.collectionName_, "getDistSearchInfo", actionItem, distResultItem.distSearchInfo_);

    distResultItem.distSearchInfo_.option_ = DistKeywordSearchInfo::OPTION_CARRIED_INFO;
#endif

    typedef std::map<workerid_t, KeywordSearchResult> ResultMapT;
    typedef ResultMapT::iterator ResultMapIterT;

    QueryIdentity identity;
    // For distributed search, as it should merge the results over all nodes,
    // the topK start offset is fixed to zero
    const uint32_t topKStart = 0;
    searchWorker_->makeQueryIdentity(identity, actionItem, distResultItem.distSearchInfo_.option_, topKStart);

    if (!searchCache_->get(identity, resultItem))
    {
        // Get and aggregate keyword search results from mutliple nodes
        distResultItem.setStartCount(actionItem.pageInfo_);

        searchAggregator_->distributeRequest(
                actionItem.collectionName_, "getDistSearchResult", actionItem, distResultItem);

        distResultItem.adjustStartCount(topKStart);

        resultItem.swap(distResultItem);
        resultItem.distSearchInfo_.nodeType_ = DistKeywordSearchInfo::NODE_MASTER;

        // Get and aggregate Summary, Mining results from multiple nodes.
        ResultMapT resultMap;
        searchMerger_->splitSearchResultByWorkerid(resultItem, resultMap);
        RequestGroup<KeywordSearchActionItem, KeywordSearchResult> requestGroup;
        for (ResultMapIterT it = resultMap.begin(); it != resultMap.end(); it++)
        {
            workerid_t workerid = it->first;
            KeywordSearchResult& subResultItem = it->second;
            requestGroup.addRequest(workerid, &actionItem, &subResultItem);
        }

        searchAggregator_->distributeRequest(
                actionItem.collectionName_, "getSummaryMiningResult", requestGroup, resultItem);

        searchCache_->set(identity, resultItem);
    }
    else
    {
        resultItem.setStartCount(actionItem.pageInfo_);
        resultItem.adjustStartCount(topKStart);

        ResultMapT resultMap;
        searchMerger_->splitSearchResultByWorkerid(resultItem, resultMap);
        RequestGroup<KeywordSearchActionItem, KeywordSearchResult> requestGroup;
        for (ResultMapIterT it = resultMap.begin(); it != resultMap.end(); it++)
        {
            workerid_t workerid = it->first;
            KeywordSearchResult& subResultItem = it->second;
            requestGroup.addRequest(workerid, &actionItem, &subResultItem);
        }

        searchAggregator_->distributeRequest(
                actionItem.collectionName_, "getSummaryResult", requestGroup, resultItem);
    }

    cout << "Total count: " << resultItem.totalCount_ << endl;
    cout << "Top K count: " << resultItem.topKDocs_.size() << endl;
    cout << "Page Count: " << resultItem.count_ << endl;

    REPORT_PROFILE_TO_FILE( "PerformanceQueryResult.SIAProcess" );

    return true;
}

bool IndexSearchService::getDocumentsByIds(
    const GetDocumentsByIdsActionItem& actionItem,
    RawTextResultFromSIA& resultItem
)
{
    if (!bundleConfig_->isMasterAggregator())
    {
        searchWorker_->getDocumentsByIds(actionItem, resultItem);
        return !resultItem.idList_.empty();
    }

    /// Perform distributed search by aggregator
    typedef std::map<workerid_t, GetDocumentsByIdsActionItem> ActionItemMapT;
    typedef ActionItemMapT::iterator ActionItemMapIterT;

    ActionItemMapT actionItemMap;
    if (!searchMerger_->splitGetDocsActionItemByWorkerid(actionItem, actionItemMap))
    {
        searchAggregator_->distributeRequest(actionItem.collectionName_, "getDocumentsByIds", actionItem, resultItem);
    }
    else
    {
        RequestGroup<GetDocumentsByIdsActionItem, RawTextResultFromSIA> requestGroup;
        for (ActionItemMapIterT it = actionItemMap.begin(); it != actionItemMap.end(); it++)
        {
            workerid_t workerid = it->first;
            GetDocumentsByIdsActionItem& subActionItem = it->second;
            requestGroup.addRequest(workerid, &subActionItem);
        }

        searchAggregator_->distributeRequest(actionItem.collectionName_, "getDocumentsByIds", requestGroup, resultItem);
    }

    return true;
}

bool IndexSearchService::getInternalDocumentId(
    const std::string& collectionName,
    const uint128_t& scdDocumentId,
    uint64_t& internalId
)
{
    internalId = 0;
    if (!bundleConfig_->isMasterAggregator())
    {
        searchWorker_->getInternalDocumentId(scdDocumentId, internalId);
    }
    else
    {
        searchAggregator_->distributeRequest<uint128_t, uint64_t>(
                collectionName, "getInternalDocumentId", scdDocumentId, internalId);
    }

    return (internalId != 0);
}

}
