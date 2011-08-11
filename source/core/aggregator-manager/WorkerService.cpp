#include "WorkerService.h"
#include "WorkerHelper.h"

#include <bundles/index/IndexSearchService.h>
#include <bundles/index/IndexBundleConfiguration.h>
#include <bundles/mining/MiningSearchService.h>
#include <bundles/recommend/RecommendSearchService.h>

#include <index-manager/IndexManager.h>
#include <search-manager/SearchManager.h>
#include <search-manager/PersonalizedSearchInfo.h>
#include <document-manager/DocumentManager.h>
#include <la-manager/LAManager.h>


namespace sf1r
{

extern int TOP_K_NUM;

WorkerService::WorkerService()
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

/// public services

bool WorkerService::processGetSearchResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    cout << "#[WorkerService::processGetSearchResult] " << actionItem.collectionName_ << endl;

    std::vector<std::vector<izenelib::util::UString> > propertyQueryTermList;

    if (! getSearchResult(const_cast<KeywordSearchActionItem&>(actionItem), resultItem, propertyQueryTermList))
    {
        return false;
    }

    resultItem.propertyQueryTermList_ = propertyQueryTermList;

    return true;
}

bool WorkerService::processGetSummaryResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    cout << "#[WorkerService::processGetSummaryResult] " << actionItem.collectionName_ << endl;

    if (!getSummaryMiningResult(const_cast<KeywordSearchActionItem&>(actionItem), resultItem, resultItem.propertyQueryTermList_))
    {
        return false;
    }

    return true;
}

bool WorkerService::processGetDocumentsByIds(const GetDocumentsByIdsActionItem& actionItem, RawTextResultFromSIA& resultItem)
{
	return true;
}

/// private methods

bool WorkerService::getSearchResult(
        KeywordSearchActionItem& actionItem,
        KeywordSearchResult& resultItem,
        std::vector<std::vector<izenelib::util::UString> >& propertyQueryTermList)
{
    CREATE_PROFILER ( searchIndex, "IndexSearchService", "processGetSearchResults: search index");

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

#if 1
        if (personalSearchInfo.enabled)
        {
            cout << "[ Got User profile by user id: " << user.idStr_ << endl;
            User::PropValueMap::iterator iter;
            for (iter = user.propValueMap_.begin(); iter != user.propValueMap_.end(); iter ++)
            {
                cout << "Item: "<< iter->first << " : " << iter->second << endl;
            }
        }
        else
        {
            cout << "[ Failed to get User profile by user id: " << user.idStr_ << endl;
        }
#endif
    }
    else
    {
        // Recommend Search Service is not available, xxx
    }

    //std::vector<std::vector<izenelib::util::UString> > propertyQueryTermList;
    if(!buildQuery(actionOperation, *bundleConfig_, propertyQueryTermList, resultItem, personalSearchInfo))
    {
        return true;
    }

    START_PROFILER ( searchIndex );
    // xxx, the page start(offset) for result is measured in results over all nodes based on sort condition.
    // In one node, we don't know how many results should be retrieved. The extreme case is, all results of
    // the requested page located in one node. (maybe can pre-fetch some statistical info to improve.)
    //int startOffset = (actionItem.pageInfo_.start_ / TOP_K_NUM) * TOP_K_NUM;
    int startOffset = 0;

    if(! searchManager_->search(
                actionOperation,
                resultItem.topKDocs_,
                resultItem.topKRankScoreList_,
                resultItem.topKCustomRankScoreList_,
                resultItem.totalCount_,
                resultItem.groupRep_,
                resultItem.attrRep_,
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
        if(!buildQuery(actionOperation, *bundleConfig_, propertyQueryTermList, resultItem, personalSearchInfo))
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
                                    TOP_K_NUM,
                                    startOffset
                                    ))
        {
            return true;
        }
    }

    // Remove duplicated docs from the result if the option is on.
    removeDuplicateDocs(actionItem, resultItem);

    //set page info in resultItem t
    resultItem.start_ = actionItem.pageInfo_.start_;
    resultItem.count_ = actionItem.pageInfo_.count_;

    //set query term and Id List
    resultItem.rawQueryString_ = actionItem.env_.queryString_;
    actionOperation.getRawQueryTermIdList(resultItem.queryTermIdList_);

    STOP_PROFILER ( searchIndex );

    DLOG(INFO) << "Total count: " << resultItem.totalCount_ << endl;
    DLOG(INFO) << "Top K count: " << resultItem.topKDocs_.size() << endl;
    DLOG(INFO) << "Page Count: " << resultItem.count_ << endl;

    return true;
}

bool WorkerService::getSummaryMiningResult(
        KeywordSearchActionItem& actionItem,
        KeywordSearchResult& resultItem,
        std::vector<std::vector<izenelib::util::UString> >& propertyQueryTermList)
{
    CREATE_PROFILER ( getSummary, "IndexSearchService", "processGetSearchResults: get raw text, snippets, summarization");
    START_PROFILER ( getSummary );

    DLOG(INFO) << "[SIAServiceHandler] RawText,Summarization,Snippet" << endl;

    if (resultItem.count_ > 0)
    {
        // id of documents in current page
        std::vector<sf1r::docid_t> docsInPage;
        std::vector<sf1r::docid_t>::iterator it = resultItem.topKDocs_.begin() + resultItem.start_; //%TOP_K_NUM;
        for(size_t i=0 ; it != resultItem.topKDocs_.end() && i<resultItem.count_; i++, it++)
        {
          docsInPage.push_back(*it);
        }
        resultItem.count_ = docsInPage.size();

        getResultItem( actionItem, docsInPage, propertyQueryTermList, resultItem);
    }

    STOP_PROFILER ( getSummary );

    REPORT_PROFILE_TO_FILE( "PerformanceQueryResult.SIAProcess" );

    cout << "[IndexSearchService] keywordSearch process Done" << endl; // XXX

    if( miningSearchService_ )
    {
        miningSearchService_->getSearchResult(resultItem);
    }

    return true;
}

void WorkerService::analyze_(const std::string& qstr, std::vector<izenelib::util::UString>& results)
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


template<typename ActionItemT, typename ResultItemT>
bool  WorkerService::getResultItem(ActionItemT& actionItem, const std::vector<sf1r::docid_t>& docsInPage,
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

bool WorkerService::removeDuplicateDocs(
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

}
