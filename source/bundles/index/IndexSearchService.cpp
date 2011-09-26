#include "IndexSearchService.h"
#include "IndexBundleHelper.h"
#include <bundles/mining/MiningSearchService.h>
#include <bundles/recommend/RecommendSearchService.h>

#include <index-manager/IndexManager.h>
#include <search-manager/SearchManager.h>
#include <search-manager/PersonalizedSearchInfo.h>
#include <search-manager/SearchCache.h>
#include <query-manager/QueryIdentity.h>
#include <ranking-manager/RankingManager.h>
#include <document-manager/DocumentManager.h>
#include <la-manager/LAManager.h>

#include <aggregator-manager/AggregatorManager.h>

#include <common/SFLogger.h>

#include <query-manager/QMCommonFunc.h>
#include <recommend-manager/User.h>
#include <common/type_defs.h>
#include <la-manager/LAPool.h>

#include <util/profiler/Profiler.h>
#include <util/profiler/ProfilerGroup.h>
#include <util/singleton.h>
#include <util/get.h>

using namespace izenelib::util;


namespace sf1r
{

IndexSearchService::IndexSearchService(IndexBundleConfiguration* config)
{
    bundleConfig_ = config;
    cache_.reset(new SearchCache(bundleConfig_->masterSearchCacheNum_));
}

IndexSearchService::~IndexSearchService()
{
}

boost::shared_ptr<AggregatorManager> IndexSearchService::getAggregatorManager()
{
    return aggregatorManager_;
}

const IndexBundleConfiguration* IndexSearchService::getBundleConfig()
{
    return bundleConfig_;
}

void IndexSearchService::OnUpdateSearchCache()
{
    cache_->clear();
}

bool IndexSearchService::getSearchResult(
    KeywordSearchActionItem& actionItem,
    KeywordSearchResult& resultItem
)
{
    CREATE_PROFILER (query, "IndexSearchService", "processGetSearchResults all: total query time");
    START_PROFILER ( query );

    if (!bundleConfig_->isSupportByAggregator())
    {
        bool ret = workerService_->doLocalSearch(actionItem, resultItem);
        std::vector<std::pair<workerid_t, KeywordSearchResult> > resultList;
        resultList.push_back(std::make_pair(0,resultItem));
        resultItem.groupRep_.convertGroupRep();
        aggregatorManager_->aggregateMiningResult(resultItem, resultList);
        STOP_PROFILER ( query );
        return ret;
    }

    /// Perform distributed search by aggregator
    DistKeywordSearchResult distResultItem;

#ifdef PREFETCH_INFO
    distResultItem.distSearchInfo_.actionType_ = DistKeywordSearchInfo::ACTION_FETCH;
    aggregatorManager_->distributeRequest<KeywordSearchActionItem, DistKeywordSearchInfo>(
            actionItem.collectionName_, "getDistSearchInfo", actionItem, distResultItem.distSearchInfo_);

    distResultItem.distSearchInfo_.actionType_ = DistKeywordSearchInfo::ACTION_SEND;
#endif

    QueryIdentity identity;
    makeQueryIdentity(identity, actionItem, distResultItem.distSearchInfo_.actionType_, actionItem.pageInfo_.start_);

    if (!cache_->get(
            identity,
            resultItem.topKRankScoreList_,
            resultItem.topKCustomRankScoreList_,
            resultItem.topKDocs_,
            resultItem.totalCount_,
            resultItem.groupRep_,
            resultItem.attrRep_,
            resultItem.propertyRange_,
            resultItem.distSearchInfo_,
            &resultItem.count_,
            &resultItem.propertyQueryTermList_,
            &resultItem.topKWorkerIds_))
    {
        // Get and aggregate keyword search results from mutliple nodes
        distResultItem.start_ = actionItem.pageInfo_.start_;
        distResultItem.count_ = actionItem.pageInfo_.count_;

        aggregatorManager_->distributeRequest(
                actionItem.collectionName_, "getDistSearchResult", actionItem, distResultItem);

        resultItem.swap(distResultItem);
        resultItem.groupRep_.convertGroupRep();

        cache_->set(
                identity,
                resultItem.topKRankScoreList_,
                resultItem.topKCustomRankScoreList_,
                resultItem.topKDocs_,
                resultItem.totalCount_,
                resultItem.groupRep_,
                resultItem.attrRep_,
                resultItem.propertyRange_,
                resultItem.distSearchInfo_,
                resultItem.count_,
                &resultItem.propertyQueryTermList_,
                &resultItem.topKWorkerIds_);
    }

    cout << "Total count: " << resultItem.totalCount_ << endl;
    cout << "Top K count: " << resultItem.topKDocs_.size() << endl;
    cout << "Page Count: " << resultItem.count_ << endl;

    // Get and aggregate Summary, Mining results from multiple nodes.
    typedef std::map<workerid_t, boost::shared_ptr<KeywordSearchResult> > ResultMapT;
    typedef std::map<workerid_t, boost::shared_ptr<KeywordSearchResult> >::iterator ResultMapIterT;

    ResultMapT resultMap;
    aggregatorManager_->splitSearchResultByWorkerid(resultItem, resultMap);
    RequestGroup<KeywordSearchActionItem, KeywordSearchResult> requestGroup;
    for (ResultMapIterT it = resultMap.begin(); it != resultMap.end(); it++)
    {
        workerid_t workerid = it->first;
        boost::shared_ptr<KeywordSearchResult>& subResultItem = it->second;
        requestGroup.addRequest(workerid, &actionItem, subResultItem.get());
    }

    aggregatorManager_->distributeRequest(
            actionItem.collectionName_, "getSummaryMiningResult", requestGroup, resultItem);

    STOP_PROFILER ( query );
    REPORT_PROFILE_TO_FILE( "PerformanceQueryResult.SIAProcess" );

    return true;
}

bool IndexSearchService::getDocumentsByIds(
    const GetDocumentsByIdsActionItem& actionItem,
    RawTextResultFromSIA& resultItem
)
{
    if (!bundleConfig_->isSupportByAggregator())
    {
        return workerService_->getDocumentsByIds(actionItem, resultItem);
    }

    /// Perform distributed search by aggregator
    typedef std::map<workerid_t, boost::shared_ptr<GetDocumentsByIdsActionItem> > ActionItemMapT;
    typedef std::map<workerid_t, boost::shared_ptr<GetDocumentsByIdsActionItem> >::iterator ActionItemMapIterT;

    ActionItemMapT actionItemMap;
    if (!aggregatorManager_->splitGetDocsActionItemByWorkerid(actionItem, actionItemMap))
    {
        aggregatorManager_->distributeRequest(actionItem.collectionName_, "getDocumentsByIds", actionItem, resultItem);
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

        aggregatorManager_->distributeRequest(actionItem.collectionName_, "getDocumentsByIds", requestGroup, resultItem);
    }

	return true;
}

bool IndexSearchService::getInternalDocumentId(
    const std::string& collectionName,
    const izenelib::util::UString& scdDocumentId,
    uint64_t& internalId
)
{
    if (!bundleConfig_->isSupportByAggregator())
    {
        return workerService_->getInternalDocumentId(scdDocumentId, internalId);
    }

    /// Perform distributed search by aggregator
    internalId = 0;
    aggregatorManager_->distributeRequest<izenelib::util::UString, uint64_t>(
            collectionName, "getInternalDocumentId", scdDocumentId, internalId);

    return (internalId != 0);
}

}

