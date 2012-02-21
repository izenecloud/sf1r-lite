#include "SearchWorker.h"
#include "WorkerHelper.h"

#include <bundles/index/IndexBundleConfiguration.h>
#include <bundles/recommend/RecommendSearchService.h>

#include <index-manager/IndexManager.h>
#include <search-manager/SearchManager.h>
#include <search-manager/SearchCache.h>
#include <search-manager/PersonalizedSearchInfo.h>
#include <document-manager/DocumentManager.h>
#include <mining-manager/MiningManager.h>
#include <la-manager/LAManager.h>


namespace sf1r
{

SearchWorker::SearchWorker(IndexBundleConfiguration* bundleConfig)
    : bundleConfig_(bundleConfig)
    , recommendSearchService_(NULL)
    , pQA_(NULL)
{
    ///LA can only be got from a pool because it is not thread safe
    ///For some situation, we need to get the la not according to the property
    ///So we had to hard code here. A better solution is to set a default
    ///LA for each collection.
    analysisInfo_.analyzerId_ = "la_sia";
    analysisInfo_.tokenizerNameList_.insert("tok_divide");
    analysisInfo_.tokenizerNameList_.insert("tok_unite");

    if (!bundleConfig_->isSupportByAggregator())
        searchCache_.reset(new SearchCache(bundleConfig_->searchCacheNum_));
}

/// Index Search

bool SearchWorker::getDistSearchInfo(const KeywordSearchActionItem& actionItem, DistKeywordSearchInfo& resultItem)
{
    DistKeywordSearchResult fakeResultItem;
    fakeResultItem.distSearchInfo_.option_ = DistKeywordSearchInfo::OPTION_GATHER_INFO;

    getSearchResult_(actionItem, fakeResultItem);

    resultItem.swap(fakeResultItem.distSearchInfo_);
    return true;
}

bool SearchWorker::getDistSearchResult(const KeywordSearchActionItem& actionItem, DistKeywordSearchResult& resultItem)
{
    cout << "[SearchWorker::processGetSearchResult] " << actionItem.collectionName_ << endl;

    return getSearchResult_(actionItem, resultItem);
}

bool SearchWorker::getSummaryMiningResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    cout << "[SearchWorker::processGetSummaryResult] " << actionItem.collectionName_ << endl;

    return getSummaryMiningResult_(actionItem, resultItem);
}

bool SearchWorker::getDocumentsByIds(const GetDocumentsByIdsActionItem& actionItem, RawTextResultFromSIA& resultItem)
{
    const izenelib::util::UString::EncodingType kEncodingType =
        izenelib::util::UString::convertEncodingTypeFromStringToEnum(
            actionItem.env_.encodingType_.c_str()
        );

    std::vector<sf1r::docid_t> idList;
    std::vector<sf1r::workerid_t> workeridList;
    actionItem.getDocWorkerIdLists(idList, workeridList);

    // append docIdList_ at the end of idList_.
    typedef std::vector<std::string>::const_iterator docid_iterator;
    izenelib::util::UString unicodeDocId;
    sf1r::docid_t internalId;
    for (docid_iterator it = actionItem.docIdList_.begin();
            it != actionItem.docIdList_.end(); ++it)
    {
        unicodeDocId.assign(*it, kEncodingType);
        if (idManager_->getDocIdByDocName(unicodeDocId, internalId, false))
        {
            idList.push_back(internalId);
        }
    }

    // get docids by property value
    collectionid_t colId = 1;
    if (!actionItem.propertyName_.empty())
    {
        std::vector<PropertyValue>::const_iterator property_value;
        for (property_value = actionItem.propertyValueList_.begin();
             property_value != actionItem.propertyValueList_.end(); ++property_value)
        {
            PropertyType value;
            PropertyValue2IndexPropertyType converter(value);
            boost::apply_visitor(converter, (*property_value).getVariant());
            indexManager_->getDocsByPropertyValue(colId, actionItem.propertyName_, value, idList);
        }
    }

    // get query terms

    izenelib::util::UString rawQueryUStr(
        izenelib::util::UString(
        actionItem.env_.queryString_, kEncodingType
        )
    );

    // Just let empty propertyQueryTermList to getResultItem() for using only raw query term list.
    vector<vector<izenelib::util::UString> > propertyQueryTermList;

    //fill rawText in result Item
    if (getResultItem(actionItem, idList, propertyQueryTermList, resultItem))
    {
        resultItem.idList_.swap(idList);
        return true;
    }

    resultItem.fullTextOfDocumentInPage_.clear();
    resultItem.snippetTextOfDocumentInPage_.clear();
    resultItem.rawTextOfSummaryInPage_.clear();
    return false;
}

bool SearchWorker::getInternalDocumentId(const izenelib::util::UString& scdDocumentId, uint64_t& internalId)
{
    uint32_t docid = 0;
    internalId = 0;
    if (!idManager_->getDocIdByDocName(scdDocumentId, docid, false))
        return false;

    internalId = docid;
    return true;
}

// local interface
bool SearchWorker::doLocalSearch(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    CREATE_PROFILER( cacheoverhead, "IndexSearchService", "cache overhead: overhead for caching in searchworker");

    START_PROFILER( cacheoverhead )
    QueryIdentity identity;
    if (searchCache_)
    {
        uint32_t TOP_K_NUM = bundleConfig_->topKNum_;
        uint32_t start = (actionItem.pageInfo_.start_ / TOP_K_NUM) * TOP_K_NUM;
        makeQueryIdentity(identity, actionItem, resultItem.distSearchInfo_.option_, start);

        if (searchCache_->get(identity, resultItem))
        {
            STOP_PROFILER( cacheoverhead )
            // the cached search results require to be reranked
            searchManager_->rerank(actionItem, resultItem);

            return true;
        }
    }

    if (! getSearchResult_(actionItem, resultItem, identity, false))
    {
        return false;
    }

    if (! getSummaryMiningResult_(actionItem, resultItem, false))
    {
        return false;
    }

    START_PROFILER( cacheoverhead )
    if (searchCache_)
        searchCache_->set(identity, resultItem);
    STOP_PROFILER( cacheoverhead )

    return true;
}

/// Mining Search

bool SearchWorker::getSimilarDocIdList(uint64_t& documentId, uint32_t& maxNum, SimilarDocIdListType& result)
{
    // todo get all similar docs in global space?
    //return miningManager_->getSimilarDocIdList(documentId, maxNum, result);

    std::pair<sf1r::workerid_t, sf1r::docid_t> wd = net::aggregator::Util::GetWorkerAndDocId(documentId);
    sf1r::workerid_t workerId = wd.first;
    sf1r::docid_t docId = wd.second;

    std::vector<std::pair<uint32_t, float> > simDocList;
    bool ret = miningManager_->getSimilarDocIdList(docId, maxNum, simDocList); //get similar docs in current machine.

    result.resize(simDocList.size());
    for (size_t i = 0; i < simDocList.size(); i ++)
    {
        uint64_t wdocid = net::aggregator::Util::GetWDocId(workerId, simDocList[i].first);
        result[i] = std::make_pair(wdocid, simDocList[i].second);
    }

    return ret;
}

bool SearchWorker::clickGroupLabel(const ClickGroupLabelActionItem& actionItem, bool& ret)
{
    ret = miningManager_->clickGroupLabel(
            actionItem.queryString_,
            actionItem.propName_,
            actionItem.groupPath_);

    return ret;
}

bool SearchWorker::visitDoc(const uint32_t& docId, bool& ret)
{
    ret = miningManager_->visitDoc(docId);
    return ret;
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

    switch (item.searchingMode_)
    {
    case SearchingMode::KNN:
        miningManager_->GetSignatureForQuery(item, identity.simHash);
        break;
    default:
        identity.query = item.env_.queryString_;
        identity.expandedQueryString = item.env_.expandedQueryString_;
        if (indexManager_->getIndexManagerConfig()->indexStrategy_.indexLevel_
                == izenelib::ir::indexmanager::DOCLEVEL
                && item.rankingType_ == RankingType::PLM)
        {
            const_cast<KeywordSearchActionItem &>(item).rankingType_ = RankingType::BM25;
        }
        identity.rankingType = item.rankingType_;
        identity.laInfo = item.languageAnalyzerInfo_;
        identity.properties = item.searchPropertyList_;
        identity.sortInfo = item.sortPriorityList_;
        identity.filterInfo = item.filteringList_;
        identity.groupParam = item.groupParam_;
        identity.rangeProperty = item.rangePropertyName_;
        identity.strExp = item.strExp_;
        identity.paramConstValueMap = item.paramConstValueMap_;
        identity.paramPropertyValueMap = item.paramPropertyValueMap_;
        identity.distActionType = distActionType;
        std::sort(identity.properties.begin(),
                identity.properties.end());
        break;
    }
}

/// private methods ////////////////////////////////////////////////////////////

template <typename ResultItemType>
bool SearchWorker::getSearchResult_(
        const KeywordSearchActionItem& actionItem,
        ResultItemType& resultItem,
        bool isDistributedSearch)
{
    QueryIdentity identity;
    return getSearchResult_(actionItem, resultItem, identity, isDistributedSearch);
}

template <typename ResultItemType>
bool SearchWorker::getSearchResult_(
        const KeywordSearchActionItem& actionItem,
        ResultItemType& resultItem,
        QueryIdentity& identity,
        bool isDistributedSearch)
{
    CREATE_SCOPED_PROFILER ( searchIndex, "IndexSearchService", "processGetSearchResults: search index");

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
    if (actionOperation.actionItem_.searchingMode_ == SearchingMode::VERBOSE)
    {
        if (pQA_->isQuestion(actionOperation.actionItem_.env_.queryString_))
        {
            analyze_(actionOperation.actionItem_.env_.queryString_, keywords, true);
            assembleConjunction(keywords, newQuery);
            //cout<<"new Query "<<newQuery<<endl;
            actionOperation.actionItem_.env_.queryString_ = newQuery;
        }
    }
    else if (actionOperation.actionItem_.searchingMode_ == SearchingMode::OR)
    {
        analyze_(actionOperation.actionItem_.env_.queryString_, keywords, false);
        assembleDisjunction(keywords, newQuery);
        actionOperation.actionItem_.env_.queryString_ = newQuery;
    }
    else if (actionOperation.actionItem_.searchingMode_ == SearchingMode::KNN)
    {
        //TODO
    }

    // Get Personalized Search information (user profile)
    PersonalSearchInfo personalSearchInfo;
    personalSearchInfo.enabled = false;
    User& user = personalSearchInfo.user;
    user.idStr_ = actionItem.env_.userID_;
    if (recommendSearchService_  && (!user.idStr_.empty()))
    {
        personalSearchInfo.enabled = recommendSearchService_->getUser(user.idStr_, user);
    }
    else
    {
        // Recommend Search Service is not available, xxx
    }

    resultItem.propertyQueryTermList_.clear();
    if (!buildQuery(actionOperation, resultItem.propertyQueryTermList_, resultItem, personalSearchInfo))
    {
        return true;
    }

    uint32_t startOffset;
    uint32_t TOP_K_NUM = bundleConfig_->topKNum_;
    uint32_t KNN_TOP_K_NUM = bundleConfig_->kNNTopKNum_;
    uint32_t KNN_DIST = bundleConfig_->kNNDist_;
    if (isDistributedSearch)
    {
        // XXX, For distributed search, the page start(offset) should be measured in results over all nodes,
        // we don't know which part of results should be retrieved in one node. Currently, the limitation of documents
        // to be retrieved in one node is set to TOP_K_NUM.
        startOffset = 0;
    }
    else
    {
        startOffset = (actionItem.pageInfo_.start_ / TOP_K_NUM) * TOP_K_NUM;
    }

    if (actionOperation.actionItem_.searchingMode_ == SearchingMode::KNN)
    {
        if (identity.simHash.empty())
            miningManager_->GetSignatureForQuery(actionOperation.actionItem_, identity.simHash);
        miningManager_->GetKNNListBySignature(
                identity.simHash,
                resultItem.topKDocs_,
                resultItem.topKRankScoreList_,
                resultItem.totalCount_,
                KNN_TOP_K_NUM,
                KNN_DIST,
                startOffset);
    }
    else if (!searchManager_->search(
                actionOperation,
                resultItem.topKDocs_,
                resultItem.topKRankScoreList_,
                resultItem.topKCustomRankScoreList_,
                resultItem.totalCount_,
                resultItem.groupRep_,
                resultItem.attrRep_,
                resultItem.propertyRange_,
                resultItem.distSearchInfo_,
                TOP_K_NUM,
                KNN_TOP_K_NUM,
                KNN_DIST,
                startOffset
                ))
    {
        std::string newQuery;

        if (!bundleConfig_->bTriggerQA_)
            return true;
        assembleDisjunction(keywords, newQuery);

        actionOperation.actionItem_.env_.queryString_ = newQuery;
        resultItem.propertyQueryTermList_.clear();
        if (!buildQuery(actionOperation, resultItem.propertyQueryTermList_, resultItem, personalSearchInfo))
        {
            return true;
        }

        if (!searchManager_->search(
                    actionOperation,
                    resultItem.topKDocs_,
                    resultItem.topKRankScoreList_,
                    resultItem.topKCustomRankScoreList_,
                    resultItem.totalCount_,
                    resultItem.groupRep_,
                    resultItem.attrRep_,
                    resultItem.propertyRange_,
                    resultItem.distSearchInfo_,
                    TOP_K_NUM,
                    KNN_TOP_K_NUM,
                    KNN_DIST,
                    startOffset
                    ))
        {
            return true;
        }
    }

    // todo, remove duplication globally over all nodes?
    // Remove duplicated docs from the result if the option is on.
    removeDuplicateDocs(actionItem, resultItem);

    //set page info in resultItem t
    resultItem.start_ = actionItem.pageInfo_.start_;
    resultItem.count_ = actionItem.pageInfo_.count_;

    std::size_t overallSearchResultSize = startOffset + resultItem.topKDocs_.size();

    if (resultItem.start_ > overallSearchResultSize)
    {
        resultItem.start_ = overallSearchResultSize;
    }
    if (resultItem.start_ + resultItem.count_ > overallSearchResultSize)
    {
        resultItem.count_ = overallSearchResultSize - resultItem.start_;
    }

    //set query term and Id List
    resultItem.rawQueryString_ = actionItem.env_.queryString_;
    actionOperation.getRawQueryTermIdList(resultItem.queryTermIdList_);

    DLOG(INFO) << "Total count: " << resultItem.totalCount_ << endl;
    DLOG(INFO) << "Top K count: " << resultItem.topKDocs_.size() << endl;
    DLOG(INFO) << "Page Count: " << resultItem.count_ << endl;

    cout << "Total count: " << resultItem.totalCount_ << endl;
    cout << "Top K count: " << resultItem.topKDocs_.size() << endl;
    cout << "Page Count: " << resultItem.count_ << endl;

    return true;
}

bool SearchWorker::getSummaryMiningResult_(
        const KeywordSearchActionItem& actionItem,
        KeywordSearchResult& resultItem,
        bool isDistributedSearch)
{
    CREATE_PROFILER ( getSummary, "IndexSearchService", "processGetSearchResults: get raw text, snippets, summarization");
    START_PROFILER ( getSummary );

    DLOG(INFO) << "[SIAServiceHandler] RawText,Summarization,Snippet" << endl;

    if (resultItem.count_ > 0)
    {
        // id of documents in current page
        std::vector<sf1r::docid_t> docsInPage;
        std::vector<sf1r::docid_t>::iterator it = resultItem.topKDocs_.begin() + resultItem.start_%bundleConfig_->topKNum_;
        for (size_t i = 0 ; it != resultItem.topKDocs_.end() && i < resultItem.count_; i++, it++)
        {
          docsInPage.push_back(*it);
        }
        resultItem.count_ = docsInPage.size();

        getResultItem( actionItem, docsInPage, resultItem.propertyQueryTermList_, resultItem);
    }

    STOP_PROFILER ( getSummary );

    cout << "[IndexSearchService] keywordSearch process Done" << endl; // XXX

    if (miningManager_)
    {
        miningManager_->getMiningResult(resultItem);
    }

    return true;
}

void SearchWorker::analyze_(const std::string& qstr, std::vector<izenelib::util::UString>& results, bool isQA)
{
    results.clear();
    izenelib::util::UString question(qstr, izenelib::util::UString::UTF_8);
    la::TermList termList;

    la::LA* pLA = LAPool::getInstance()->popSearchLA( analysisInfo_);
//    pLA->process_search(question, termList);
    if (!pLA) return;
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

template <typename ResultItemT>
bool SearchWorker::buildQuery(
    SearchKeywordOperation& actionOperation,
    std::vector<std::vector<izenelib::util::UString> >& propertyQueryTermList,
    ResultItemT& resultItem,
    PersonalSearchInfo& personalSearchInfo
)
{
    if (actionOperation.actionItem_.searchingMode_ == SearchingMode::KNN)
        return true;

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
    resultItem.analyzedQuery_.resize(kPropertyList.size());

    std::string convertBuffer;
    for (size_t i = 0; i < kPropertyList.size(); i++)
    {
        const std::string& propertyName = kPropertyList[i];
        const PropertyTermInfo& propertyTermInfo = izenelib::util::getOr(
            kPropertyTermInfoMap,
            propertyName,
            kDefaultPropertyTermInfo
        );

        DLOG(INFO) << propertyTermInfo.toString();
        propertyTermInfo.getPositionedTermStringList(
            propertyQueryTermList[i]
        );

        // Get analyzed Query
        propertyTermInfo.getPositionedFullTermString(
            resultItem.analyzedQuery_[i]
        );
        resultItem.analyzedQuery_[i].convertString(
            convertBuffer, resultItem.encodingType_
        );

        DLOG(INFO) << "-------[ Analyzed Query for " << propertyName << " : \"" << convertBuffer << "\"" << endl;
    }

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

    resultItem.snippetTextOfDocumentInPage_.resize(
            actionItem.displayPropertyList_.size()
            );
    resultItem.fullTextOfDocumentInPage_.resize(
            actionItem.displayPropertyList_.size()
            );

    // shrink later
    resultItem.rawTextOfSummaryInPage_.resize(
            actionItem.displayPropertyList_.size()
            );


    UString::EncodingType encodingType( UString::convertEncodingTypeFromStringToEnum( (actionItem.env_.encodingType_).c_str() ));
    UString rawQueryUStr(actionItem.env_.queryString_, encodingType );

    bool containsOriginalTermsOnly = false;
    if ( propertyQueryTermList.size() != actionItem.displayPropertyList_.size() )
        containsOriginalTermsOnly = true;

    typedef std::vector<DisplayProperty>::size_type vec_size_type;
    // counter for properties requiring summary, later
    vec_size_type indexSummary = 0;
    bool ret = true;
    for (vec_size_type i = 0; i < actionItem.displayPropertyList_.size(); ++i)
    {
        //add raw + analyzed + tokenized query terms for snippet and highlight algorithms
        std::vector<izenelib::util::UString> queryTerms;

        if ( containsOriginalTermsOnly )
            QueryUtility::getMergedUniqueTokens(
                    rawQueryUStr,
                    laManager_,
                    queryTerms,
                    actionItem.languageAnalyzerInfo_.useOriginalKeyword_
                    );
        else
            QueryUtility::getMergedUniqueTokens(
                    propertyQueryTermList[i],
                    rawQueryUStr,
                    laManager_,
                    queryTerms,
                    actionItem.languageAnalyzerInfo_.useOriginalKeyword_);

        //analyze_(actionItem.env_.queryString_, queryTerms);


        // propertyOption

        if (!actionItem.env_.taxonomyLabel_.empty())
           queryTerms.insert(queryTerms.begin(), UString(actionItem.env_.taxonomyLabel_, encodingType ) );

        if (!actionItem.env_.nameEntityItem_.empty())
           queryTerms.insert(queryTerms.begin(),UString( actionItem.env_.nameEntityItem_, encodingType ) );

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
            ret &=  documentManager_->getRawTextOfDocuments(
                    docsInPage,
                    actionItem.displayPropertyList_[i].propertyString_,
                    propertyOption,
                    queryTerms,
                    resultItem.snippetTextOfDocumentInPage_[i],
                    resultItem.fullTextOfDocumentInPage_[i]
                    );
        }

    } // for each displayPropertyList

    // indexSummary now is the real size of the array
    resultItem.rawTextOfSummaryInPage_.resize(indexSummary);
    if (!ret)
        resultItem.error_="Error : Cannot get document data";

    return ret;
}

template <typename ResultItemType>
bool SearchWorker::removeDuplicateDocs(
        const KeywordSearchActionItem& actionItem,
        ResultItemType& resultItem
)
{
    if (actionItem.searchingMode_ == SearchingMode::KNN)
        return true;

    // Remove duplicated docs from the result if the option is on.
    if (miningManager_)
    {
      if ( actionItem.removeDuplicatedDocs_ && (resultItem.topKDocs_.size() != 0) )
      {
          std::vector<sf1r::docid_t> dupRemovedDocs;
          bool ret = miningManager_->getUniqueDocIdList(resultItem.topKDocs_, dupRemovedDocs);
          if ( ret )
          {
              resultItem.topKDocs_.swap(dupRemovedDocs);
          }
      }
    }
    return true;
}

void SearchWorker::reset_cache(
        bool rType,
        docid_t id,
        const std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue)
{
    //this method is only used for r-type filed right now
    searchCache_->clear();
    searchManager_->reset_cache(rType, id, rTypeFieldValue);
}

void SearchWorker::reset_all_property_cache()
{
    searchCache_->clear();
    searchManager_->reset_all_property_cache();
}


}
