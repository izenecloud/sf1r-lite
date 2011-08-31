#include "IndexSearchService.h"
#include "IndexBundleHelper.h"
#include <bundles/mining/MiningSearchService.h>
#include <bundles/recommend/RecommendSearchService.h>

#include <index-manager/IndexManager.h>
#include <search-manager/SearchManager.h>
#include <search-manager/PersonalizedSearchInfo.h>
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

#define DISTRIBUTED_SEARCH

namespace sf1r
{
int TOP_K_NUM = 4000;

IndexSearchService::IndexSearchService()
{
    miningSearchService_ = NULL;
    recommendSearchService_ = NULL;
    ///LA can only be got from a pool because it is not thread safe
    ///For some situation, we need to get the la not according to the property
    ///So we had to hard code here. A better solution is to set a default
    ///LA for each collection.
    analysisInfo_.analyzerId_ = "la_sia";
    analysisInfo_.tokenizerNameList_.insert("tok_divide");
    analysisInfo_.tokenizerNameList_.insert("tok_unite");
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

#ifdef DISTRIBUTED_SEARCH

    /// Perform search on local node, if collection was not supported by aggregator.
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

    aggregatorManager_->distributeRequest<KeywordSearchActionItem, DistKeywordSearchResult>(
            actionItem.collectionName_, "getDistSearchResult", actionItem, distResultItem);

    resultItem.assign(distResultItem);

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

    //requestGroup.setResultItemForMerging(&resultItem);

    aggregatorManager_->distributeRequest<KeywordSearchActionItem, KeywordSearchResult>(
            actionItem.collectionName_, "getSummaryMiningResult", requestGroup, resultItem);

    //resultItem.print();//:~
    return true;

    STOP_PROFILER ( query );

    REPORT_PROFILE_TO_FILE( "PerformanceQueryResult.SIAProcess" );

#else
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
    if(bundleConfig_->bTriggerQA_)
    {
        if(pQA_->isQuestion(actionOperation.actionItem_.env_.queryString_))
        {
            analyze_(actionOperation.actionItem_.env_.queryString_, keywords);
            assembleConjunction(keywords, newQuery);
            //cout<<"new Query "<<newQuery<<endl;
            actionOperation.actionItem_.env_.queryString_ = newQuery;
        }
    }

    // Get Personalized Search information (user profile)
    PersonalSearchInfo personalSearchInfo;
    personalSearchInfo.enabled = false;

    User& user = personalSearchInfo.user;
    user.idStr_ = actionItem.env_.userID_;
    if( recommendSearchService_  && (!user.idStr_.empty()))
    {
        personalSearchInfo.enabled = recommendSearchService_->getUser(user.idStr_, user);
    }
    else
    {
        // Recommend Search Service is not available, xxx
    }

    std::vector<std::vector<izenelib::util::UString> > propertyQueryTermList;
    if(!buildQuery(actionOperation, propertyQueryTermList, resultItem, personalSearchInfo))
    {
        return true;
    }

    START_PROFILER ( searchIndex );
    int startOffset = (actionItem.pageInfo_.start_ / TOP_K_NUM) * TOP_K_NUM;

    if(! searchManager_->search(
                actionOperation,
                resultItem.topKDocs_,
                resultItem.topKRankScoreList_,
                resultItem.topKCustomRankScoreList_,
                resultItem.totalCount_,
                resultItem.groupRep_,
                resultItem.attrRep_,
                resultItem.propertyRange_,
                TOP_K_NUM,
                startOffset
                ))
    {
        std::string newQuery;

        if(!bundleConfig_->bTriggerQA_)
            return true;
        assembleDisjunction(keywords, newQuery);

        actionOperation.actionItem_.env_.queryString_ = newQuery;
        propertyQueryTermList.clear();
        if(!buildQuery(actionOperation, propertyQueryTermList, resultItem, personalSearchInfo))
        {
            return true;
        }

        if(! searchManager_->search(
                                    actionOperation,
                                    resultItem.topKDocs_,
                                    resultItem.topKRankScoreList_,
                                    resultItem.topKCustomRankScoreList_,
                                    resultItem.totalCount_,
                                    resultItem.groupRep_,
                                    resultItem.attrRep_,
                                    resultItem.propertyRange_,
                                    TOP_K_NUM,
                                    startOffset
                                    ))
        {
            return true;
        }
    }

    // Remove duplicated docs from the result if the option is on.
    removeDuplicateDocs(actionItem, resultItem);

    //set top K document in resultItem t
    resultItem.start_ = actionItem.pageInfo_.start_;
    resultItem.count_ = actionItem.pageInfo_.count_;

    std::size_t overallSearchResultSize = startOffset + resultItem.topKDocs_.size();

    if(resultItem.start_ > overallSearchResultSize)
    {
        resultItem.start_ = overallSearchResultSize;
    }
    else if(resultItem.start_ + resultItem.count_ > overallSearchResultSize)
    {
        resultItem.count_ = overallSearchResultSize - resultItem.start_;
    }

    //set query term and Id List
    resultItem.rawQueryString_ = actionItem.env_.queryString_;
    actionOperation.getRawQueryTermIdList(resultItem.queryTermIdList_);

    STOP_PROFILER ( searchIndex );

    DLOG(INFO) << "Total count: " << resultItem.totalCount_ << endl;
    DLOG(INFO) << "Top K count: " << resultItem.topKDocs_.size() << endl;
    DLOG(INFO) << "Page Count: " << resultItem.count_ << endl;

    DLOG(INFO) << "[SIAServiceHandler] RawText,Summarization,Snippet" << endl;

    START_PROFILER ( getSummary );
    if (resultItem.count_ > 0)
    {
        // id of documents in current page
        std::vector<sf1r::docid_t> docsInPage;
        std::vector<sf1r::docid_t>::iterator it = resultItem.topKDocs_.begin() + resultItem.start_%TOP_K_NUM;
        for(size_t i=0 ; it != resultItem.topKDocs_.end() && i<resultItem.count_; i++, it++)
        {
          docsInPage.push_back(*it);
        }
        resultItem.count_ = docsInPage.size();

        getResultItem( actionItem, docsInPage, propertyQueryTermList, resultItem);
    }

    STOP_PROFILER ( getSummary );
    STOP_PROFILER ( query );
    REPORT_PROFILE_TO_FILE( "PerformanceQueryResult.SIAProcess" );

    cout << "[IndexSearchService] keywordSearch process Done" << endl; // XXX

    if( miningSearchService_ )
    {
        miningSearchService_->getSearchResult(resultItem);

        if (actionItem.env_.isLogGroupLabels_)
        {
            const faceted::GroupParam::GroupLabelVec& groupLabels = actionItem.groupParam_.groupLabels_;
            for (faceted::GroupParam::GroupLabelVec::const_iterator it = groupLabels.begin();
                it != groupLabels.end(); ++it)
            {
                if (miningSearchService_->clickGroupLabel(actionItem.env_.queryString_, it->first, it->second) == false)
                {
                    LOG(ERROR) << "error in log group label click, query: " << actionItem.env_.queryString_
                               << ", property name: " << it->first << ", path size: " << it->second.size();
                }
            }
        }
    }

    return true;

#endif

}

void IndexSearchService::analyze_(const std::string& qstr, std::vector<izenelib::util::UString>& results)
{
    results.clear();
    izenelib::util::UString question(qstr, izenelib::util::UString::UTF_8);
    la::TermList termList;

    la::LA* pLA = LAPool::getInstance()->popSearchLA( analysisInfo_);
//    pLA->process_search(question, termList);
    if(!pLA) return;
    pLA->process(question, termList);
    LAPool::getInstance()->pushSearchLA( analysisInfo_, pLA );

    std::string str;
    for(la::TermList::iterator iter = termList.begin(); iter != termList.end(); ++iter)
    {
        iter->text_.convertString(str, izenelib::util::UString::UTF_8);

        if(! pQA_->isQuestionTerm(iter->text_))
        {
            if(pQA_->isCandidateTerm(iter->pos_))
            {
                results.push_back(iter->text_);
                cout<<" la-> "<<str<<endl;
            }
        }
    }
}

bool IndexSearchService::buildQuery(
    SearchKeywordOperation& actionOperation,
    std::vector<std::vector<izenelib::util::UString> >& propertyQueryTermList,
    KeywordSearchResult& resultItem,
    PersonalSearchInfo& personalSearchInfo
)
{
    CREATE_PROFILER ( buildQuery, "IndexSearchService", "processGetSearchResults: build query tree");
    CREATE_PROFILER ( analyzeQuery, "IndexSearchService", "processGetSearchResults: analyze query");

    propertyQueryTermList.resize(0);
    resultItem.analyzedQuery_.resize(0);

    START_PROFILER ( buildQuery );
    std::string errorMessage;
    bool buildSuccess = buildQueryTree(actionOperation, *bundleConfig_, resultItem.error_, personalSearchInfo);
    STOP_PROFILER ( buildQuery );

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
bool  IndexSearchService::getResultItem(ActionItemT& actionItem, const std::vector<sf1r::docid_t>& docsInPage,
        const vector<vector<izenelib::util::UString> >& propertyQueryTermList, ResultItemT&  resultItem)
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
        
		if( actionItem.env_.taxonomyLabel_ != "" )
		   queryTerms.insert(queryTerms.begin(), UString(actionItem.env_.taxonomyLabel_, encodingType ) );
		
		if( actionItem.env_.nameEntityItem_ != "" )
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
    if(!ret)
        resultItem.error_="Error : Cannot get document data";

    return ret;
}

bool IndexSearchService::removeDuplicateDocs(
    KeywordSearchActionItem& actionItem,
    KeywordSearchResult& resultItem
)
{
    // Remove duplicated docs from the result if the option is on.
    if( miningSearchService_)
    {
      if ( actionItem.removeDuplicatedDocs_ && (resultItem.topKDocs_.size() != 0) )
      {
          std::vector<sf1r::docid_t> dupRemovedDocs;
          bool ret = miningSearchService_->getUniqueDocIdList(resultItem.topKDocs_, dupRemovedDocs);
          if ( ret )
          {
              resultItem.topKDocs_.swap(dupRemovedDocs);
          }
      }
    }
    return true;
}

bool IndexSearchService::getDocumentsByIds(
    const GetDocumentsByIdsActionItem& actionItem, 
    RawTextResultFromSIA& resultItem
)
{
#ifdef DISTRIBUTED_SEARCH

    /// Perform search on local node, if collection was not supported by aggregator.
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

#else

    const izenelib::util::UString::EncodingType kEncodingType =
        izenelib::util::UString::convertEncodingTypeFromStringToEnum(
            actionItem.env_.encodingType_.c_str()
        );

    std::vector<sf1r::docid_t> idList;
    std::vector<sf1r::workerid_t> workeridList;
    const_cast<GetDocumentsByIdsActionItem&>(actionItem).getDocWorkerIdLists(idList, workeridList);

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

    BitVector* bitVector = indexManager_->getIndexReader()->getDocFilter();
    if (bitVector)
    {
        vector<sf1r::docid_t> tmpIdList;
        for(size_t i = 0; i < idList.size(); i++)
        {
            if(!bitVector->test(idList[i]))
                tmpIdList.push_back(idList[i]);
        }

        idList.swap(tmpIdList);
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

#endif
}

bool IndexSearchService::getInternalDocumentId(
    const std::string& collectionName,
    const izenelib::util::UString& scdDocumentId, 
    uint64_t& internalId
)
{
#ifdef DISTRIBUTED_SEARCH

    /// Perform search on local node, if collection was not supported by aggregator.
    if (!SF1Config::get()->checkAggregatorSupport(collectionName))
    {
        return workerService_->getInternalDocumentId(scdDocumentId, internalId);
    }

    /// Perform distributed search by aggregator
    internalId = 0;
    aggregatorManager_->distributeRequest<izenelib::util::UString, uint64_t>(
            collectionName, "getInternalDocumentId", scdDocumentId, internalId);

    return (internalId != 0);

#else
    uint32_t docid;
    std::string str;
    scdDocumentId.convertString(str, izenelib::util::UString::UTF_8);
    internalId = 0;
    if (idManager_->getDocIdByDocName(str, docid, false))
    {
        internalId = docid;
        return true;
    }
    return false;
#endif
}

}

