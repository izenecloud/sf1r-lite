#include "SearchWorker.h"
#include "WorkerHelper.h"

#include <bundles/index/IndexBundleConfiguration.h>

#include <common/SearchCache.h>
#include <common/Utilities.h>
#include <common/QueryNormalizer.h>
#include <index-manager/InvertedIndexManager.h>
#include <search-manager/SearchManager.h>
#include <search-manager/SearchBase.h>
#include <search-manager/PersonalizedSearchInfo.h>
#include <search-manager/QueryPruneFactory.h>
#include <search-manager/QueryPruneBase.h>
#include <search-manager/QueryHelper.h>
#include <document-manager/DocumentManager.h>
#include <mining-manager/MiningManager.h>
#include <la-manager/LAManager.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/MasterManagerBase.h>
#include <util/driver/Request.h>
#include <aggregator-manager/MasterNotifier.h>
#include <query-manager/QueryTypeDef.h>
#include <search-manager/GeoHashEncoder.h>

namespace sf1r
{

SearchWorker::SearchWorker(IndexBundleConfiguration* bundleConfig)
    : bundleConfig_(bundleConfig)
    , searchCache_(new SearchCache(bundleConfig_->searchCacheNum_,
                                    bundleConfig_->refreshCacheInterval_,
                                    bundleConfig_->refreshSearchCache_))
    , pQA_(NULL)
    , queryPruneFactory_(new QueryPruneFactory())
{
    ///LA can only be got from a pool because it is not thread safe
    ///For some situation, we need to get the la not according to the property
    ///So we had to hard code here. A better solution is to set a default
    ///LA for each collection.
    analysisInfo_.analyzerId_ = "la_sia";
    analysisInfo_.tokenizerNameList_.insert("tok_divide");
    analysisInfo_.tokenizerNameList_.insert("tok_unite");
}

void SearchWorker::HookDistributeRequestForSearch(int hooktype, const std::string& reqdata, bool& result)
{
    MasterManagerBase::get()->pushWriteReq(reqdata, "api_from_shard");
    //DistributeRequestHooker::get()->setHook(hooktype, reqdata);
    //DistributeRequestHooker::get()->hookCurrentReq(reqdata);
    //DistributeRequestHooker::get()->processLocalBegin();
    LOG(INFO) << "got hook request on the search worker." << reqdata;
    result = true;
}

/// Index Search
void SearchWorker::getDistSearchInfo(const KeywordSearchActionItem& actionItem, DistKeywordSearchInfo& resultItem)
{
    KeywordSearchResult fakeResultItem;
    fakeResultItem.distSearchInfo_.swap(resultItem);

    getSearchResult_(actionItem, fakeResultItem);

    resultItem.swap(fakeResultItem.distSearchInfo_);
}

void SearchWorker::getDistSearchResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    LOG(INFO) << "[SearchWorker::processGetSearchResult] " << actionItem.collectionName_ << endl;

    getSearchResult_(actionItem, resultItem);
    resultItem.rawQueryString_ = actionItem.env_.queryString_;

    if (!resultItem.topKDocs_.empty())
        searchManager_->topKReranker_.rerank(actionItem, resultItem);

    if (!actionItem.disableGetDocs_)
    {
        if( (resultItem.topKDocs_.size() > 20) &&
            (actionItem.pageInfo_.start_ + actionItem.pageInfo_.count_) > 20 )
        {
            return;
        }
        std::vector<sf1r::docid_t> possible_docsInPage;
        if (actionItem.pageInfo_.count_ > 0)
        {
            std::vector<sf1r::docid_t>::iterator it = resultItem.topKDocs_.begin();
            for (size_t i = 0 ; it != resultItem.topKDocs_.end(); ++i, ++it)
            {
                if (i < actionItem.pageInfo_.start_ + actionItem.pageInfo_.count_)
                {
                    possible_docsInPage.push_back(*it);
                }
                else
                    break;
            }
        }
        LOG(INFO) << "pre get documents since the page result is small. size: " << possible_docsInPage.size();

        resultItem.distSearchInfo_.include_summary_data_ = true;
        // SearchMerger::splitSearchResultByWorkerid has put current page docs into "topKDocs_"
        getResultItem(actionItem, possible_docsInPage, resultItem.propertyQueryTermList_, resultItem);
    }
}

void SearchWorker::getSummaryResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    LOG(INFO) << "[SearchWorker::processGetSummaryResult] " << actionItem.collectionName_
      << ", query: " << actionItem.env_.queryString_ << endl;

    getSummaryResult_(actionItem, resultItem);
}

void SearchWorker::getSummaryMiningResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    getSummaryMiningResult_(actionItem, resultItem);
}

void SearchWorker::getDocumentsByIds(const GetDocumentsByIdsActionItem& actionItem, RawTextResultFromSIA& resultItem)
{
    const izenelib::util::UString::EncodingType kEncodingType =
        izenelib::util::UString::convertEncodingTypeFromStringToEnum(actionItem.env_.encodingType_.c_str());

    std::vector<sf1r::docid_t> idList;
    std::vector<sf1r::workerid_t> workeridList;
    actionItem.getDocWorkerIdLists(idList, workeridList);

    LOG(INFO) << "get docs by ids, idlist: " << idList.size() << ", docidlist:" << actionItem.docIdList_.size();
    // append docIdList_ at the end of idList_.
    typedef std::vector<std::string>::const_iterator docid_iterator;
    sf1r::docid_t internalId;
    for (docid_iterator it = actionItem.docIdList_.begin();
            it != actionItem.docIdList_.end(); ++it)
    {
        if (idManager_->getDocIdByDocName(Utilities::md5ToUint128(*it), internalId, false))
        {
            if(!documentManager_->isDeleted(internalId))
                idList.push_back(internalId);
            else
                LOG(INFO) << "doc is deleted while try to get." << *it << ", " << internalId;
        }
        else
        {
            LOG(INFO) << "get doc not found: " << *it;
        }
    }

    LOG(INFO) << "after convert, get docs by ids idlist: " << idList.size();
    // get docids by property value
    //collectionid_t colId = 1;
    if (!actionItem.propertyName_.empty())
    {
        std::vector<PropertyValue>::const_iterator property_value;
        for (property_value = actionItem.propertyValueList_.begin();
                property_value != actionItem.propertyValueList_.end(); ++property_value)
        {
            PropertyType value;
            PropertyValue2IndexPropertyType converter(value);
            boost::apply_visitor(converter, (*property_value).getVariant());
            invertedIndexManager_->getDocsByPropertyValue(actionItem.propertyName_, value, idList);
        }
        LOG(INFO) << "after get docs from property, ids idlist: " << idList.size();
    }

    // get query terms

    izenelib::util::UString rawQueryUStr(actionItem.env_.queryString_, kEncodingType);

    // Just let empty propertyQueryTermList to getResultItem() for using only raw query term list.
    vector<vector<izenelib::util::UString> > propertyQueryTermList;

    //fill rawText in result Item
    if (getResultItem(actionItem, idList, propertyQueryTermList, resultItem))
    {
        resultItem.idList_.swap(idList);
    }
}

void SearchWorker::getInternalDocumentId(const uint128_t& scdDocumentId, uint64_t& internalId)
{
    uint32_t docid = 0;
    internalId = 0;

    if (idManager_->getDocIdByDocName(scdDocumentId, docid, false))
        internalId = docid;
}

void SearchWorker::rerank(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    // this is used for distributed search.
    // we can rerank the result if the merged result has the rerank data with them.
    //searchManager_->topKReranker_.rerank(actionItem, resultItem);
}

// local interface
bool SearchWorker::doLocalSearch(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    CREATE_PROFILER( cacheoverhead, "IndexSearchService", "cache overhead: overhead for caching in searchworker");

    START_PROFILER( cacheoverhead )

    uint32_t TOP_K_NUM = bundleConfig_->topKNum_;
    uint32_t topKStart = actionItem.pageInfo_.topKStart(TOP_K_NUM, IsTopKComesFromConfig(actionItem));

    QueryIdentity identity;
    makeQueryIdentity(identity, actionItem, resultItem.distSearchInfo_.option_, topKStart);

    if (!searchCache_->get(identity, resultItem))
    {
        STOP_PROFILER( cacheoverhead )

        if (! getSearchResult_(actionItem, resultItem, identity, false))
            return false;

        resultItem.rawQueryString_ = actionItem.env_.queryString_;

        if (!resultItem.topKDocs_.empty() && actionItem.searchingMode_.mode_ != SearchingMode::AD_INDEX)
            searchManager_->topKReranker_.rerank(actionItem, resultItem);

        if (resultItem.topKDocs_.empty())
            return true;


        if (! getSummaryMiningResult_(actionItem, resultItem, false))
            return false;

        START_PROFILER( cacheoverhead )
        if (searchCache_)
            searchCache_->set(identity, resultItem);
        STOP_PROFILER( cacheoverhead )
    }
    else
    {
        STOP_PROFILER( cacheoverhead );

        if (actionItem.searchingMode_.mode_ == SearchingMode::AD_INDEX)
        {
            if (miningManager_->getAdIndexManager())
            {
                resultItem.topKDocs_.swap(resultItem.adCachedTopKDocs_);
                miningManager_->getAdIndexManager()->rankAndSelect(
                    std::vector<std::pair<std::string, std::string> >(),
                    resultItem.topKDocs_, resultItem.topKRankScoreList_, resultItem.totalCount_);
            }
        }
        resultItem.setStartCount(actionItem.pageInfo_);
        resultItem.adjustStartCount(topKStart);

        if (! getSummaryResult_(actionItem, resultItem, false))
            return false;
    }

    return true;
}

void SearchWorker::clickGroupLabel(const ClickGroupLabelActionItem& actionItem, bool& result)
{
    result = miningManager_->clickGroupLabel(
            actionItem.queryString_,
            actionItem.propName_,
            actionItem.groupPath_);
}

void SearchWorker::visitDoc(const uint32_t& docId, bool& result)
{
    result = miningManager_->visitDoc(docId);
    //if (DistributeRequestHooker::get()->getHookType() == izenelib::driver::Request::FromDistribute)
    //{
    //    // for rpc call from master, we can not decide whether the caller is from
    //    // local or remote master, so we assume all caller is remote.
    //    // In order to make sure the hook is cleared, we need return true to
    //    // tell caller that we will take the charge to clear any hooked on this request.
    //    DistributeRequestHooker::get()->processLocalFinished(result);
    //    result = true;
    //    LOG(INFO) << " set remote result to true on this write request : " << __FUNCTION__;
    //}
}

void SearchWorker::makeQueryIdentity(
        QueryIdentity& identity,
        const KeywordSearchActionItem& item,
        int8_t distActionType,
        uint32_t start)
{
    identity.userId = item.env_.userID_;
    identity.start = start;
    identity.searchingMode = item.searchingMode_;
    identity.isSynonym = item.languageAnalyzerInfo_.synonymExtension_;

	//using geohash-encode string as part of cache-key
	//geohash 7bytes represents about 70m scope, 8bytes represents about 20m
	//if the center coordinate in the same geohash grid we can approximatly consider they are same.
	//Maybe 70m is not reasonable, we can do some adjustment. 
	if(!item.geoLocationProperty_.empty()){
		GeoHashEncoder encoder;
		identity.geoLocation = item.geoLocation_;
		identity.geohash = encoder.Encoder(item.geoLocation_.first, 
										   item.geoLocation_.second, 
										   7);
	}

    switch (item.searchingMode_.mode_)
    {
    case SearchingMode::SUFFIX_MATCH:
        identity.query = item.env_.queryString_;
        identity.properties = item.searchPropertyList_;
        identity.filterTree = item.filterTree_;
        identity.sortInfo = item.sortPriorityList_;
        identity.strExp = item.strExp_;
        identity.paramConstValueMap = item.paramConstValueMap_;
        identity.paramPropertyValueMap = item.paramPropertyValueMap_;
        identity.groupParam = item.groupParam_;
        identity.isRandomRank = item.isRandomRank_;
        identity.querySource = item.env_.querySource_;
        identity.distActionType = distActionType;
        identity.isAnalyzeResult = item.isAnalyzeResult_;
        break;
    case SearchingMode::ZAMBEZI:
        identity.query = item.env_.queryString_;
        identity.properties = item.searchPropertyList_;
        identity.filterTree = item.filterTree_;
        identity.sortInfo = item.sortPriorityList_;
        identity.strExp = item.strExp_;
        identity.paramConstValueMap = item.paramConstValueMap_;
        identity.paramPropertyValueMap = item.paramPropertyValueMap_;
        identity.groupParam = item.groupParam_;
        identity.isRandomRank = item.isRandomRank_;
        identity.querySource = item.env_.querySource_;
        identity.distActionType = distActionType;
		identity.scope = item.scope_;
        break;
    default:
        identity.query = item.env_.queryString_;
        identity.expandedQueryString = item.env_.expandedQueryString_;
        if (invertedIndexManager_->getIndexManagerConfig()->indexStrategy_.indexLevel_
                == izenelib::ir::indexmanager::DOCLEVEL
                && item.rankingType_ == RankingType::PLM)
        {
            const_cast<KeywordSearchActionItem &>(item).rankingType_ = RankingType::BM25;
        }
        identity.rankingType = item.rankingType_;
        identity.laInfo = item.languageAnalyzerInfo_;
        identity.properties = item.searchPropertyList_;
        identity.counterList = item.counterList_;
        identity.sortInfo = item.sortPriorityList_;
        identity.filterTree = item.filterTree_;
        identity.groupParam = item.groupParam_;
        identity.removeDuplicatedDocs = item.removeDuplicatedDocs_;
        identity.rangeProperty = item.rangePropertyName_;
        identity.strExp = item.strExp_;
        identity.paramConstValueMap = item.paramConstValueMap_;
        identity.paramPropertyValueMap = item.paramPropertyValueMap_;
        identity.distActionType = distActionType;
        identity.isRandomRank = item.isRandomRank_;
        identity.querySource = item.env_.querySource_;
        std::sort(identity.properties.begin(),
                identity.properties.end());
        std::sort(identity.counterList.begin(),
                identity.counterList.end());
        break;
    }
}

/// private methods ////////////////////////////////////////////////////////////

bool SearchWorker::getSearchResult_(
        const KeywordSearchActionItem& actionItem,
        KeywordSearchResult& resultItem,
        bool isDistributedSearch)
{
    QueryIdentity identity;
    uint32_t TOP_K_NUM = bundleConfig_->topKNum_;
    uint32_t topKStart = actionItem.pageInfo_.topKStart(TOP_K_NUM, IsTopKComesFromConfig(actionItem));
    makeQueryIdentity(identity, actionItem, resultItem.distSearchInfo_.option_, topKStart);
    return getSearchResult_(actionItem, resultItem, identity, isDistributedSearch);
}

bool SearchWorker::getSearchResult_(
        const KeywordSearchActionItem& actionItem,
        KeywordSearchResult& resultItem,
        QueryIdentity& identity,
        bool isDistributedSearch)
{
    CREATE_SCOPED_PROFILER ( searchIndex, "IndexSearchService", "processGetSearchResults: search index");

    time_t start_search = time(NULL);
    // Set basic info for response
    resultItem.collectionName_ = actionItem.collectionName_;
    resultItem.encodingType_ =
        izenelib::util::UString::convertEncodingTypeFromStringToEnum(
            actionItem.env_.encodingType_.c_str()
        );
    SearchKeywordOperation actionOperation(actionItem, bundleConfig_->isUnigramWildcard(),
            laManager_, idManager_);
    actionOperation.hasUnigramProperty_ = bundleConfig_->hasUnigramProperty();
    actionOperation.isUnigramSearchMode_ = bundleConfig_->isUnigramSearchMode();

    std::vector<izenelib::util::UString> keywords;
    std::string newQuery;
    if (actionOperation.actionItem_.searchingMode_.mode_ == SearchingMode::VERBOSE)
    {
        if (pQA_->isQuestion(actionOperation.actionItem_.env_.queryString_))
        {
            analyze_(actionOperation.actionItem_.env_.queryString_, keywords, true);
            assembleConjunction(keywords, newQuery);
            //cout<<"new Query "<<newQuery<<endl;
            actionOperation.actionItem_.env_.queryString_ = newQuery;
        }
    }
    else if (actionOperation.actionItem_.searchingMode_.mode_ == SearchingMode::OR)
    {
        analyze_(actionOperation.actionItem_.env_.queryString_, keywords, false);
        assembleDisjunction(keywords, newQuery);
        actionOperation.actionItem_.env_.queryString_ = newQuery;
    }


    // Get Personalized Search information (user profile)
    PersonalSearchInfo personalSearchInfo;
    personalSearchInfo.enabled = false;
    User& user = personalSearchInfo.user;
    user.idStr_ = actionItem.env_.userID_;

    resultItem.propertyQueryTermList_.clear();
    if (!buildQuery(actionOperation, resultItem.propertyQueryTermList_, resultItem, personalSearchInfo))
    {
        return true;
    }

    actionOperation.getRawQueryTermIdList(resultItem.queryTermIdList_);

    uint32_t topKStart = 0;
    uint32_t TOP_K_NUM = bundleConfig_->topKNum_;
    uint32_t fuzzy_lucky = actionOperation.actionItem_.searchingMode_.lucky_;

    uint32_t search_limit = TOP_K_NUM;
    resultItem.TOP_K_NUM = TOP_K_NUM;

    // XXX, For distributed search, the page start(offset) should be measured in results over all nodes,
    // we don't know which part of results should be retrieved in one node. Currently, the limitation of documents
    // to be retrieved in one node is set to TOP_K_NUM.
    if (isDistributedSearch)
    {
        // distributed search need get more topk since
        // each worker can only start topk from 0.
        search_limit += actionOperation.actionItem_.pageInfo_.start_;
    }
    else
    {
        topKStart = identity.start;
    }

    LOG(INFO) << "searching in mode: " << actionOperation.actionItem_.searchingMode_.mode_;

    // if (actionOperation.actionItem_.filterTree_)
    // {
    //     actionOperation.actionItem_.filterTree_->printConditionInfo();
    // }

    std::vector<QueryFiltering::FilteringType> filteringRules;
    if (actionOperation.actionItem_.searchingMode_.mode_ == SearchingMode::SUFFIX_MATCH)
        actionOperation.actionItem_.filterTree_.getFilteringListSuffix(filteringRules);

    switch (actionOperation.actionItem_.searchingMode_.mode_)
    {
    case SearchingMode::SUFFIX_MATCH:
        resultItem.TOP_K_NUM = fuzzy_lucky;
        if (!miningManager_->GetSuffixMatch(actionOperation,
                                            fuzzy_lucky,
                                            actionOperation.actionItem_.searchingMode_.usefuzzy_,
                                            topKStart,
                                            filteringRules,
                                            resultItem.topKDocs_,
                                            resultItem.topKRankScoreList_,
                                            resultItem.topKCustomRankScoreList_,
                                            resultItem.topKGeoDistanceList_,
                                            resultItem.totalCount_,
                                            resultItem.groupRep_,
                                            resultItem.attrRep_,
                                            actionOperation.actionItem_.isAnalyzeResult_,
                                            resultItem.analyzedQuery_,
                                            resultItem.pruneQueryString_,
                                            resultItem.distSearchInfo_,
                                            resultItem.autoSelectGroupLabels_))
        {
            return true;
        }

        break;

    case SearchingMode::ZAMBEZI:
        if (!searchManager_->zambeziSearch_->search(actionOperation,
                                                    resultItem,
                                                    search_limit,
                                                    topKStart))
        {
            return true;
        }

        break;

    case SearchingMode::AD_INDEX:
        if (!miningManager_->getAdIndexManager()->searchByQuery(
                    actionOperation, resultItem))
        {
            return true;
        }
        break;

    default:
        unsigned int QueryPruneTimes = 2;
        bool isUsePrune = false;
        //isUsePrune = actionOperation.actionItem_.searchingMode_.useQueryPrune_;
        bool is_getResult = true;
        if (!searchManager_->normalSearch_->search(actionOperation,
                                                   resultItem,
                                                   search_limit,
                                                   topKStart) || resultItem.totalCount_ == 0)
        {
            cout<<"resultItem.totalCount_:"<<resultItem.totalCount_<<endl;
            if (time(NULL) - start_search > 5)
            {
                LOG(INFO) << "search cost too long : " << start_search << " , " << time(NULL);
                actionOperation.actionItem_.print();
            }

            /// muti thread ....
            /// * star search
            const bool isFilterQuery =
            actionOperation.rawQueryTree_->type_ == QueryTree::FILTER_QUERY;

            if (isFilterQuery)
                return true;

            /// query frune
            QueryPruneBase* queryPrunePtr = NULL;
            QueryPruneType qrType;

            if ( isUsePrune == true)
            {
                qrType = AND_TRIGGER;
            }
            else if (bundleConfig_->bTriggerQA_)
                qrType = QA_TRIGGER;
            else
                return true;

            RequesterEnvironment& requestEnv = actionOperation.actionItem_.env_;
            std::string rawQuery = requestEnv.queryString_;

            do
            {
                std::string newQuery = "";
                queryPrunePtr = queryPruneFactory_->getQueryPrune(qrType);

                if (queryPrunePtr != NULL)
                {
                    if(!queryPrunePtr->queryPrune(rawQuery, keywords, newQuery))
                    {
                        LOG(INFO) << "There is no Prune query";
                        return true;
                    }
                }
                LOG(INFO) << "[QUERY PRUNE] the new query is" <<  newQuery << endl;
                requestEnv.queryString_ = newQuery;
                QueryNormalizer::get()->normalize(requestEnv.queryString_,
                                                  requestEnv.normalizedQueryString_);

                resultItem.propertyQueryTermList_.clear();
                if (!buildQuery(actionOperation, resultItem.propertyQueryTermList_, resultItem, personalSearchInfo))
                {
                    return true;
                }
                QueryPruneTimes--;
                is_getResult =  searchManager_->normalSearch_->search(actionOperation,
                                                                      resultItem,
                                                                      search_limit,
                                                                      topKStart);
                rawQuery = newQuery;
            } while(isUsePrune && QueryPruneTimes > 0 && (!is_getResult || resultItem.totalCount_ == 0));
        }
        break;
    }

    if (time(NULL) - start_search > 5)
    {
        LOG(INFO) << "search cost too long : " << start_search << " , " << time(NULL);
        actionOperation.actionItem_.print();
    }

    // For non-distributed search, it's necessary to adjust "start_" and "count_"
    // For distributed search, they would be adjusted in IndexSearchService::getSearchResult()
    if (!isDistributedSearch)
    {
        resultItem.setStartCount(actionItem.pageInfo_);
        resultItem.adjustStartCount(topKStart);
    }

    actionOperation.getRawQueryTermIdList(resultItem.queryTermIdList_);

    DLOG(INFO) << "Total count: " << resultItem.totalCount_ << endl;
    DLOG(INFO) << "Top K count: " << resultItem.topKDocs_.size() << endl;
    DLOG(INFO) << "Page Count: " << resultItem.count_ << endl;

    cout << "Total count: " << resultItem.totalCount_ << endl;
    cout << "Top K count: " << resultItem.topKDocs_.size() << endl;
    cout << "Page Count: " << resultItem.count_ << endl;

    return true;
}

bool SearchWorker::getSummaryResult_(
        const KeywordSearchActionItem& actionItem,
        KeywordSearchResult& resultItem,
        bool isDistributedSearch)
{
    if (resultItem.count_ == 0 || actionItem.disableGetDocs_)
        return true;

    CREATE_PROFILER ( getSummary, "IndexSearchService", "processGetSearchResults: get raw text, snippets, summarization");
    START_PROFILER ( getSummary );

    LOG(INFO) << "Begin get RawText,Summarization,Snippet" << endl;

    if (isDistributedSearch)
    {
        // SearchMerger::splitSearchResultByWorkerid has put current page docs into "topKDocs_"
        getResultItem(actionItem, resultItem.docsInPage_, resultItem.propertyQueryTermList_, resultItem);
    }
    else
    {
        // get current page docs
        std::vector<sf1r::docid_t> docsInPage;
        if ((resultItem.start_ % resultItem.TOP_K_NUM) < resultItem.topKDocs_.size())
        {
            std::vector<sf1r::docid_t>::iterator it = resultItem.topKDocs_.begin() + resultItem.start_% resultItem.TOP_K_NUM;
            for (size_t i = 0 ; it != resultItem.topKDocs_.end() && i < resultItem.count_; i++, it++)
            {
                docsInPage.push_back(*it);
            }
        }
        resultItem.count_ = docsInPage.size();

        getResultItem(actionItem, docsInPage, resultItem.propertyQueryTermList_, resultItem);
    }

    STOP_PROFILER ( getSummary );

    LOG(INFO) << "getSummaryResult_ done." << endl; // XXX

    return true;
}

bool SearchWorker::getSummaryMiningResult_(
        const KeywordSearchActionItem& actionItem,
        KeywordSearchResult& resultItem,
        bool isDistributedSearch)
{
    LOG(INFO) << "[SearchWorker::processGetSummaryMiningResult] " << actionItem.collectionName_ << endl;

    getSummaryResult_(actionItem, resultItem, isDistributedSearch);
    LOG(INFO) << "[SearchWorker::processGetSummaryMiningResult] finished.";
    return true;
}

void SearchWorker::analyze_(const std::string& qstr, std::vector<izenelib::util::UString>& results, bool isQA)
{
    la::LA* pLA = LAPool::getInstance()->popSearchLA( analysisInfo_);
//    pLA->process_search(question, termList);
    if (!pLA) return;
    results.clear();
    izenelib::util::UString question(qstr, izenelib::util::UString::UTF_8);
    la::TermList termList;
    pLA->process(question, termList);
    LAPool::getInstance()->pushSearchLA( analysisInfo_, pLA );

    std::string str;
    for (la::TermList::iterator iter = termList.begin(); iter != termList.end(); ++iter)
    {
        iter->text_.convertString(str, izenelib::util::UString::UTF_8);
        if (isQA)
        {
            if (!pQA_->isQuestionTerm(iter->text_))
            {
                if (pQA_->isCandidateTerm(iter->pos_))
                {
                    results.push_back(iter->text_);
                    cout<<" la-> "<<str<<endl;
                }
            }
        }
        else
        {
            results.push_back(iter->text_);
        }
    }
}

bool SearchWorker::buildQuery(
        SearchKeywordOperation& actionOperation,
        std::vector<std::vector<izenelib::util::UString> >& propertyQueryTermList,
        KeywordSearchResult& resultItem,
        PersonalSearchInfo& personalSearchInfo)
{
    SearchingMode::SearchingModeType mode = actionOperation.actionItem_.searchingMode_.mode_;
    if (mode == SearchingMode::SUFFIX_MATCH ||
        mode == SearchingMode::ZAMBEZI)
    {
        return true;
    }

    CREATE_PROFILER ( constructQueryTree, "IndexSearchService", "processGetSearchResults: build query tree");
    CREATE_PROFILER ( analyzeQuery, "IndexSearchService", "processGetSearchResults: analyze query");

    propertyQueryTermList.resize(0);
    resultItem.analyzedQuery_.resize(0);

    START_PROFILER ( constructQueryTree );
    std::string errorMessage;
    bool buildSuccess = buildQueryTree(actionOperation, *bundleConfig_, resultItem.error_, personalSearchInfo);
    STOP_PROFILER ( constructQueryTree );

    if (!buildSuccess)
    {
        return false;
    }
    // Get queries for each property and build propertyTermInfo
    // ----------------------------------------------------------
    START_PROFILER ( analyzeQuery );

    // PropertyTermInfomap contains property term frequency and position of each
    // query.
    const std::map<std::string, PropertyTermInfo>& kPropertyTermInfoMap
        = actionOperation.getPropertyTermInfoMap();
    static const PropertyTermInfo kDefaultPropertyTermInfo;

    const std::vector<std::string>& kPropertyList =
        actionOperation.actionItem_.searchPropertyList_;

    propertyQueryTermList.resize(kPropertyList.size());

    //for (size_t i = 0; i < kPropertyList.size(); i++)
    //{
    const std::string& propertyName = kPropertyList[0];
    const PropertyTermInfo& propertyTermInfo = izenelib::util::getOr(
        kPropertyTermInfoMap,
        propertyName,
        kDefaultPropertyTermInfo
    );

    DLOG(INFO) << propertyTermInfo.toString();
    propertyTermInfo.getPositionedTermStringList(
        propertyQueryTermList[0]
    );

    // Get analyzed Query
    propertyTermInfo.getPositionedFullTermString(
        resultItem.analyzedQuery_
    );

    //}

    STOP_PROFILER ( analyzeQuery );

    return true;
}

template<typename ActionItemT, typename ResultItemT>
bool  SearchWorker::getResultItem(
        ActionItemT& actionItem,
        const std::vector<sf1r::docid_t>& docsInPage,
        const vector<vector<izenelib::util::UString> >& propertyQueryTermList,
        ResultItemT& resultItem)
{
    using izenelib::util::UString;

    //boost::mutex::scoped_lock lock(mutex_);

    resultItem.snippetTextOfDocumentInPage_.resize(actionItem.displayPropertyList_.size());
    resultItem.fullTextOfDocumentInPage_.resize(actionItem.displayPropertyList_.size());

    // shrink later
    resultItem.rawTextOfSummaryInPage_.resize(actionItem.displayPropertyList_.size());

    UString::EncodingType encodingType(UString::convertEncodingTypeFromStringToEnum(actionItem.env_.encodingType_.c_str()));
    UString rawQueryUStr(actionItem.env_.queryString_, encodingType);

    std::vector<izenelib::util::UString> queryTerms;

    QueryUtility::getMergedUniqueTokens(
            rawQueryUStr,
            laManager_,
            queryTerms,
            actionItem.languageAnalyzerInfo_.useOriginalKeyword_
    );

    //queryTerms is begin segmented after analyze_()
    bool isRequireHighlight = false;
    for(std::vector<DisplayProperty>::const_iterator it = actionItem.displayPropertyList_.begin();
        it != actionItem.displayPropertyList_.end(); ++it)
    {
        if(it->isHighlightOn_ ||it->isSnippetOn_)
        {
            isRequireHighlight = true;
            break;
        }
    }
    if(isRequireHighlight)
        analyze_(actionItem.env_.queryString_, queryTerms, false);

    ///get documents at first, so that those documents will all exist in cache.
    ///To be optimized !!!:
    ///summary/snipet/highlight should utlize the extracted documents object, instead of get once more
    ///ugly design currently
    std::map<docid_t, int> doc_idx_map;
    const unsigned int docListSize = docsInPage.size();
    std::vector<unsigned int> ids(docListSize);

    for (unsigned int i=0; i<docListSize; i++)
    {
        docid_t docId = docsInPage[i];
        doc_idx_map[docId] = i;
        ids[i] = docId;
    }

    typedef std::vector<DisplayProperty>::size_type vec_size_type;
    vec_size_type indexSummary = 0;
    std::vector<Document> docs;
    bool forceget = (miningManager_&&miningManager_->HasDeletedDocDuringMining())||bundleConfig_->enable_forceget_doc_;
    if(!documentManager_->getDocuments(ids, docs, forceget))
    {
        ///Whenever any document could not be retrieved, return false
        resultItem.error_ = "Error : Cannot get document data";
        LOG(WARNING) << "get document data failed. doc num: " << ids.size() << ", is forceget:" << forceget;
        if (ids.size() < 10)
        {
            for(size_t i = 0; i < ids.size(); ++i)
                std::cout << " " << ids[i] << ",";
        }
        std::cout << std::endl;
        for (vec_size_type i = 0; i < actionItem.displayPropertyList_.size(); ++i)
        {
            if (actionItem.displayPropertyList_[i].isSummaryOn_)
                indexSummary++;
        }
        resultItem.rawTextOfSummaryInPage_.resize(indexSummary);
        return false;
    }

    /// start to get snippet/summary/highlight
    // counter for properties requiring summary, later
    bool ret = true;
    for (vec_size_type i = 0; i < actionItem.displayPropertyList_.size(); ++i)
    {
        //add raw + analyzed + tokenized query terms for snippet and highlight algorithms

        unsigned propertyOption = 0;
        if (actionItem.displayPropertyList_[i].isHighlightOn_)
        {
            propertyOption |= 1;
        }
        if (actionItem.displayPropertyList_[i].isSnippetOn_)
        {
            propertyOption |= 2;
        }
        if (actionItem.displayPropertyList_[i].isSummaryOn_)
        {
            ///To be optimized
            ret &=  documentManager_->getRawTextOfDocuments(
                    docsInPage,
                    actionItem.displayPropertyList_[i].propertyString_,
                    actionItem.displayPropertyList_[i].isSummaryOn_,
                    actionItem.displayPropertyList_[i].summarySentenceNum_,
                    propertyOption,
                    queryTerms,
                    resultItem.snippetTextOfDocumentInPage_[i],
                    resultItem.rawTextOfSummaryInPage_[indexSummary],
                    resultItem.fullTextOfDocumentInPage_[i]
                    );
            indexSummary++;
        }
        else
        {
            resultItem.snippetTextOfDocumentInPage_[i].resize(docListSize);
            resultItem.fullTextOfDocumentInPage_[i].resize(docListSize);
            std::map<docid_t, int>::const_iterator dit = doc_idx_map.begin();
            for (size_t ii = 0; dit != doc_idx_map.end(); ++dit,++ii)
            {
                documentManager_->getRawTextOfOneDocument(
                    dit->first,
                    docs[dit->second],
                    actionItem.displayPropertyList_[i].propertyString_,
                    propertyOption,
                    queryTerms,
                    resultItem.snippetTextOfDocumentInPage_[i][dit->second],
                    resultItem.fullTextOfDocumentInPage_[i][dit->second]);
            }
            ret = true;
        }

    } // for each displayPropertyList

    // indexSummary now is the real size of the array
    resultItem.rawTextOfSummaryInPage_.resize(indexSummary);
    if (!ret)
        resultItem.error_ = "Error : Cannot get document data";

    return ret;
}

void SearchWorker::reset_all_property_cache()
{
    clearSearchCache();

    clearFilterCache();
}

void SearchWorker::clearSearchCache()
{
    searchCache_->clear();
    LOG(INFO) << "notify master to clear cache.";
    if (bundleConfig_->isWorkerNode())
    {
        NotifyMSG msg;
        msg.collection = bundleConfig_->collectionName_;
        msg.method = "CLEAR_SEARCH_CACHE";
        MasterNotifier::get()->notify(msg);
    }
}

void SearchWorker::clearFilterCache()
{
    searchManager_->queryBuilder_->reset_cache();
}

uint32_t SearchWorker::getDocNum()
{
    return documentManager_->getNumDocs();
}

uint32_t SearchWorker::getKeyCount(const std::string& property_name)
{
    if (bundleConfig_->isNormalSchemaEnable_)
        return invertedIndexManager_->getBTreeIndexer()->count(property_name);
    return 0;
}

}
