#include "IndexSearchService.h"

#include <search-manager/SearchCache.h>
#include <aggregator-manager/SearchWorker.h>
#include <aggregator-manager/SearchAggregator.h>

#include <common/SFLogger.h>
#include <common/type_defs.h>

namespace sf1r
{

IndexSearchService::IndexSearchService(IndexBundleConfiguration* config)
    : bundleConfig_(config)
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
        std::vector<std::pair<workerid_t, KeywordSearchResult> > resultList;
        resultList.push_back(std::make_pair(0, resultItem));
        searchAggregator_->aggregateMiningResult(resultItem, resultList);
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

    QueryIdentity identity;
    searchWorker_->makeQueryIdentity(identity, actionItem, distResultItem.distSearchInfo_.option_, actionItem.pageInfo_.start_);

    if (!searchCache_->get(identity, resultItem))
    {
        // Get and aggregate keyword search results from mutliple nodes
        distResultItem.start_ = actionItem.pageInfo_.start_;
        distResultItem.count_ = actionItem.pageInfo_.count_;

        searchAggregator_->distributeRequest(
                actionItem.collectionName_, "getDistSearchResult", actionItem, distResultItem);

        resultItem.swap(distResultItem);
        resultItem.distSearchInfo_.nodeType_ = DistKeywordSearchInfo::NODE_MASTER;

        // Get and aggregate Summary, Mining results from multiple nodes.
        typedef std::map<workerid_t, boost::shared_ptr<KeywordSearchResult> > ResultMapT;
        typedef std::map<workerid_t, boost::shared_ptr<KeywordSearchResult> >::iterator ResultMapIterT;

        ResultMapT resultMap;
        searchAggregator_->splitSearchResultByWorkerid(resultItem, resultMap);
        RequestGroup<KeywordSearchActionItem, KeywordSearchResult> requestGroup;
        for (ResultMapIterT it = resultMap.begin(); it != resultMap.end(); it++)
        {
            workerid_t workerid = it->first;
            boost::shared_ptr<KeywordSearchResult>& subResultItem = it->second;
            requestGroup.addRequest(workerid, &actionItem, subResultItem.get());
        }

        searchAggregator_->distributeRequest(
                actionItem.collectionName_, "getSummaryMiningResult", requestGroup, resultItem);

        searchCache_->set(identity, resultItem);
    }
    else
    {
        typedef std::map<workerid_t, boost::shared_ptr<KeywordSearchResult> > ResultMapT;
        typedef std::map<workerid_t, boost::shared_ptr<KeywordSearchResult> >::iterator ResultMapIterT;

        ResultMapT resultMap;
        searchAggregator_->splitSearchResultByWorkerid(resultItem, resultMap);
        RequestGroup<KeywordSearchActionItem, KeywordSearchResult> requestGroup;
        for (ResultMapIterT it = resultMap.begin(); it != resultMap.end(); it++)
        {
            workerid_t workerid = it->first;
            boost::shared_ptr<KeywordSearchResult>& subResultItem = it->second;
            requestGroup.addRequest(workerid, &actionItem, subResultItem.get());
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
        return searchWorker_->getDocumentsByIds(actionItem, resultItem);
    }

    /// Perform distributed search by aggregator
    typedef std::map<workerid_t, boost::shared_ptr<GetDocumentsByIdsActionItem> > ActionItemMapT;
    typedef std::map<workerid_t, boost::shared_ptr<GetDocumentsByIdsActionItem> >::iterator ActionItemMapIterT;

    ActionItemMapT actionItemMap;
    if (!searchAggregator_->splitGetDocsActionItemByWorkerid(actionItem, actionItemMap))
    {
        searchAggregator_->distributeRequest(actionItem.collectionName_, "getDocumentsByIds", actionItem, resultItem);
    }
    else
    {
        RequestGroup<GetDocumentsByIdsActionItem, RawTextResultFromSIA> requestGroup;
        for (ActionItemMapIterT it = actionItemMap.begin(); it != actionItemMap.end(); it++)
        {
            workerid_t workerid = it->first;
            boost::shared_ptr<GetDocumentsByIdsActionItem>& subActionItem = it->second;
            requestGroup.addRequest(workerid, subActionItem.get());
        }

        searchAggregator_->distributeRequest(actionItem.collectionName_, "getDocumentsByIds", requestGroup, resultItem);
    }

    return true;
}

bool IndexSearchService::getInternalDocumentId(
    const std::string& collectionName,
    const izenelib::util::UString& scdDocumentId,
    uint64_t& internalId
)
{
    if (!bundleConfig_->isMasterAggregator())
    {
        return searchWorker_->getInternalDocumentId(scdDocumentId, internalId);
    }

    /// Perform distributed search by aggregator
    internalId = 0;
    searchAggregator_->distributeRequest<izenelib::util::UString, uint64_t>(
            collectionName, "getInternalDocumentId", scdDocumentId, internalId);

    return (internalId != 0);
}

}
