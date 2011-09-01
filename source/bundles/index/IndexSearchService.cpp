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
#include <process/common/XmlConfigParser.h>

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
int TOP_K_NUM = 4000;

IndexSearchService::IndexSearchService(IndexBundleConfiguration* config)
{
    cache_.reset(new SearchCache(config->searchCacheNum_));
}

IndexSearchService::~IndexSearchService()
{
}

bool IndexSearchService::getSearchResult(
    KeywordSearchActionItem& actionItem, 
    KeywordSearchResult& resultItem
)
{
    CREATE_PROFILER (query, "IndexSearchService", "processGetSearchResults all: total query time");
    START_PROFILER ( query );

    if (!SF1Config::get()->checkAggregatorSupport(actionItem.collectionName_))
    {
        return workerService_->doLocalSearch(actionItem, resultItem);
    }

    /// Perform distributed search by aggregator
    // set basic info for result
    resultItem.collectionName_ = actionItem.collectionName_;
    resultItem.encodingType_ = izenelib::util::UString::convertEncodingTypeFromStringToEnum(
            actionItem.env_.encodingType_.c_str() );
    resultItem.rawQueryString_ = actionItem.env_.queryString_;
    resultItem.start_ = actionItem.pageInfo_.start_;
    resultItem.count_ = actionItem.pageInfo_.count_;

    // Get and aggregate keyword results from mutliple nodes
    DistKeywordSearchResult distResultItem;
    distResultItem.start_ = actionItem.pageInfo_.start_;
    distResultItem.count_ = actionItem.pageInfo_.count_;

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
        aggregatorManager_->distributeRequest<KeywordSearchActionItem, DistKeywordSearchResult>(
                actionItem.collectionName_, "getDistSearchResult", actionItem, distResultItem);

        resultItem.assign(distResultItem);

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

    //resultItem.print();//:~

    // Get and aggregate Summary, Mining results from multiple nodes.
    std::map<workerid_t, boost::shared_ptr<KeywordSearchResult> > resultMap;
    aggregatorManager_->splitSearchResultByWorkerid(resultItem, resultMap);

    RequestGroup<KeywordSearchActionItem, KeywordSearchResult> requestGroup;
    std::map<workerid_t, boost::shared_ptr<KeywordSearchResult> >::iterator it;
    for (it = resultMap.begin(); it != resultMap.end(); it++)
    {
        workerid_t workerid = it->first;
        boost::shared_ptr<KeywordSearchResult>& subResultItem = it->second;

        requestGroup.addRequest(workerid, &actionItem, subResultItem.get());
    }

    aggregatorManager_->distributeRequest<KeywordSearchActionItem, KeywordSearchResult>(
            actionItem.collectionName_, "getSummaryMiningResult", requestGroup, resultItem);

    //resultItem.print();//:~

    STOP_PROFILER ( query );
    REPORT_PROFILE_TO_FILE( "PerformanceQueryResult.SIAProcess" );

    return true;
}

bool IndexSearchService::getDocumentsByIds(
    const GetDocumentsByIdsActionItem& actionItem, 
    RawTextResultFromSIA& resultItem
)
{
    if (!SF1Config::get()->checkAggregatorSupport(actionItem.collectionName_))
    {
        return workerService_->getDocumentsByIds(actionItem, resultItem);
    }

    /// Perform distributed search by aggregator
    std::map<workerid_t, boost::shared_ptr<GetDocumentsByIdsActionItem> > actionItemMap;
    if (!aggregatorManager_->splitGetDocsActionItemByWorkerid(actionItem, actionItemMap))
    {
        aggregatorManager_->distributeRequest<GetDocumentsByIdsActionItem, RawTextResultFromSIA>(
                actionItem.collectionName_, "getDocumentsByIds", actionItem, resultItem);
    }
    else
    {
        RequestGroup<GetDocumentsByIdsActionItem, RawTextResultFromSIA> requestGroup;

        std::map<workerid_t, boost::shared_ptr<GetDocumentsByIdsActionItem> >::iterator it;
        for (it = actionItemMap.begin(); it != actionItemMap.end(); it++)
        {
            workerid_t workerid = it->first;
            boost::shared_ptr<GetDocumentsByIdsActionItem>& subActionItem = it->second;

            requestGroup.addRequest(workerid, subActionItem.get());
        }

        //requestGroup.setResultItemForMerging(&resultItem);

        aggregatorManager_->distributeRequest<GetDocumentsByIdsActionItem, RawTextResultFromSIA>(
                actionItem.collectionName_, "getDocumentsByIds", requestGroup, resultItem);
    }

	return true;
}

bool IndexSearchService::getInternalDocumentId(
    const std::string& collectionName,
    const izenelib::util::UString& scdDocumentId, 
    uint64_t& internalId
)
{
    if (!SF1Config::get()->checkAggregatorSupport(collectionName))
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

