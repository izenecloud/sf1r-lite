/**
 * @file process/controllers/DocumentsSearchHandler.cpp
 * @author Ian Yang
 * @date Created <2010-06-04 13:47:46>
 */

// Must include date_time before hlmalloc.h in UString, which defines
// many macros with very short and common names and change internal code of
// boost::date_time
#include <boost/date_time.hpp>

#include <bundles/index/IndexSearchService.h>
#include <bundles/mining/MiningSearchService.h>

#include "DocumentsSearchHandler.h"

#include <parsers/SelectParser.h>
#include <parsers/FilteringParser.h>
#include <parsers/GroupingParser.h>
#include <parsers/AttrParser.h>
#include <parsers/RangeParser.h>
#include <parsers/SearchParser.h>
#include <parsers/SortParser.h>
#include <parsers/CustomRankingParser.h>
#include <common/BundleSchemaHelpers.h>

#include <common/Keys.h>
#include <common/parsers/PageInfoParser.h>

#include <log-manager/UserQuery.h>
#include <query-manager/QueryManager.h>

#include <util/swap.h>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string/split.hpp>

#include <utility>
#include <algorithm>
#include <functional>

namespace sf1r
{
using driver::Keys;
using namespace izenelib::driver;

namespace detail
{
struct DocumentsSearchKeywordsLogger
{
    DocumentsSearchKeywordsLogger()
            : start_(boost::posix_time::microsec_clock::local_time())
    {}

    void log(const KeywordSearchActionItem& actionItem,
             const KeywordSearchResult& searchResult,
             bool success);

private:
    boost::posix_time::ptime start_;
};
} // namespace detail

const int DocumentsSearchHandler::kRefinedQueryThreshold = 0;

DocumentsSearchHandler::DocumentsSearchHandler(
    ::izenelib::driver::Request& request,
    ::izenelib::driver::Response& response,
    const CollectionHandler& collectionHandler
)
        : request_(request),
        response_(response),
        indexSearchService_(collectionHandler.indexSearchService_),
        miningSearchService_(collectionHandler.miningSearchService_),
        indexSchema_(collectionHandler.indexSchema_),
        miningSchema_(collectionHandler.miningSchema_),
        actionItem_(),
        returnAnalyzerResult_(false),
        TOP_K_NUM(collectionHandler.indexSearchService_->getBundleConfig()->topKNum_)
{
    actionItem_.env_.encodingType_ = "UTF-8";
    actionItem_.env_.ipAddress_ = request.header()[Keys::remote_ip].getString();
    actionItem_.collectionName_ = asString(request[Keys::collection]);
    renderer_.TOP_K_NUM = TOP_K_NUM;
}

void DocumentsSearchHandler::search()
{
    if (parse())
    {
        addAclFilters();
        KeywordSearchResult searchResult;

        int startOffset = (actionItem_.pageInfo_.start_ / TOP_K_NUM) * TOP_K_NUM;

        if (actionItem_.env_.taxonomyLabel_.empty()
            && actionItem_.env_.nameEntityItem_.empty())
        {
            // initialize before search to record start time.
            detail::DocumentsSearchKeywordsLogger keywordsLogger;
            if (doSearch(searchResult))
            {
                response_[Keys::total_count] = searchResult.totalCount_;
                response_[Keys::top_k_count] = startOffset + searchResult.topKDocs_.size();

                renderDocuments(searchResult);
                renderMiningResult(searchResult);
                renderRangeResult(searchResult);
                renderRefinedQuery();
            }

            try
            {
                keywordsLogger.log(actionItem_, searchResult, response_.success());
            }
            catch (const std::exception& e)
            {
                DLOG(ERROR) << "[documents/search] Failed to log keywords: "
                << e.what() << std::endl;
            }
        }
        else
        {
            // search label/ne in result

            // Page Info is used to get raw text for documents with the
            // specified label.

            unsigned start = actionItem_.pageInfo_.start_ - startOffset;
            unsigned count = actionItem_.pageInfo_.count_;

            // DO NOT get raw text.
            actionItem_.pageInfo_.start_ = startOffset;
            actionItem_.pageInfo_.count_ = 0;

            if (doSearch(searchResult))
            {
                GetDocumentsByIdsActionItem getActionItem;
                RawTextResultFromMIA rawTextResult;
                std::size_t totalCount = 0;

                if (!actionItem_.env_.taxonomyLabel_.empty())
                {
                    totalCount = getDocumentIdListInLabel(
                                     searchResult,
                                     start,
                                     count,
                                     getActionItem.idList_
                                 );
                }
                else if (!actionItem_.env_.nameEntityItem_.empty())
                {
                    totalCount = getDocumentIdListInNameEntityItem(
                                     searchResult,
                                     start,
                                     count,
                                     getActionItem.idList_
                                 );
                }
                else
                {
                    response_.addError("Invalid search in label.");
                    return;
                }

                bool isSuccess = true;
                if (!getActionItem.idList_.empty())
                {
                    getActionItem.env_ = actionItem_.env_;
                    getActionItem.languageAnalyzerInfo_ =
                        actionItem_.languageAnalyzerInfo_;
                    getActionItem.collectionName_ = actionItem_.collectionName_;
                    getActionItem.displayPropertyList_
                    = actionItem_.displayPropertyList_;

                    if (doGet(getActionItem, searchResult, rawTextResult))
                    {
                        renderDocuments(rawTextResult);
                        renderMiningResult(searchResult);
                        renderRangeResult(searchResult);
                        renderRefinedQuery();
                    }
                    else
                    {
                        isSuccess = false;
                    }
                }

                if (isSuccess)
                {
                    // top_k_count equals to total_count when search in label/ne
                    response_[Keys::total_count] = totalCount;
                    response_[Keys::top_k_count] = totalCount;
                }
            }
        }
    }
}

std::size_t DocumentsSearchHandler::getDocumentIdListInLabel(
    const KeywordSearchResult& miaResult,
    unsigned start,
    unsigned count,
    std::vector<sf1r::wdocid_t>& idListInPage
)
{
    izenelib::util::UString taxonomyLabel(
        actionItem_.env_.taxonomyLabel_,
        izenelib::util::UString::UTF_8
    );
    typedef std::vector<izenelib::util::UString>::const_iterator iterator;
    std::size_t taxonomyIndex =
        std::find(
            miaResult.taxonomyString_.begin(),
            miaResult.taxonomyString_.end(),
            taxonomyLabel
        ) - miaResult.taxonomyString_.begin();

    std::size_t totalCount = 0;
    if (taxonomyIndex < miaResult.tgDocIdList_.size())
    {
        const std::vector<sf1r::wdocid_t>& tgDocIdList =
            miaResult.tgDocIdList_[taxonomyIndex];
        totalCount = tgDocIdList.size();

        std::size_t end = start + count;
        if (end > tgDocIdList.size())
        {
            end = tgDocIdList.size();
        }
        for (std::size_t i = start; i < end; ++i)
        {
            idListInPage.push_back(tgDocIdList[i]);
        }
    }

    return totalCount;
}

std::size_t DocumentsSearchHandler::getDocumentIdListInNameEntityItem(
    const KeywordSearchResult& miaResult,
    unsigned start,
    unsigned count,
    std::vector<sf1r::wdocid_t>& idListInPage
)
{
    izenelib::util::UString type(
        actionItem_.env_.nameEntityType_,
        izenelib::util::UString::UTF_8
    );
    izenelib::util::UString name(
        actionItem_.env_.nameEntityItem_,
        izenelib::util::UString::UTF_8
    );

    std::size_t totalCount = 0;
    typedef NEResultList::const_iterator ne_result_list_iterator;
    ne_result_list_iterator resultOfType =
        std::find_if(miaResult.neList_.begin(), miaResult.neList_.end(),
                     boost::bind(&NEResult::type, _1) == type);
    if (resultOfType != miaResult.neList_.end())
    {
        typedef std::vector<NEItem>::const_iterator item_iterator;
        item_iterator foundItem =
            std::find_if(resultOfType->item_list.begin(),
                         resultOfType->item_list.end(),
                         boost::bind(&NEItem::text, _1) == name);

        if (foundItem != resultOfType->item_list.end())
        {
            totalCount = foundItem->doc_list.size();
            std::size_t end = start + count;
            if (end > totalCount)
            {
                end = totalCount;
            }
            for (std::size_t i = start; i < end; ++i)
            {
                idListInPage.push_back(foundItem->doc_list[i]);
            }
        }
    }

    return totalCount;
}

void DocumentsSearchHandler::filterDocIdList(const KeywordSearchResult& origin, const std::vector<sf1r::docid_t>& id_list, KeywordSearchResult& new_result)
{

}

bool DocumentsSearchHandler::doGet(
    const GetDocumentsByIdsActionItem& getActionItem,
    const KeywordSearchResult& miaResult,
    RawTextResultFromMIA& rawTextResult
)
{
    bool requestSent = indexSearchService_->getDocumentsByIds(getActionItem, rawTextResult);
    if (!requestSent)
    {
        response_.addError("Internal communication error.");
        return false;
    }
    //add dd result
    rawTextResult.numberOfDuplicatedDocs_.resize(rawTextResult.idList_.size(), 0);
//     uint32_t pos1=0,pos2=0;
//     while(pos1<miaResult.topKDocs_.size() && pos<rawTextResult.idList_.size() )
//     {
//
//     }
//     for(uint32_t i=0;i<rawTextResult.idList_.size();i++)
//     {
//         wdocid_t wdocid = rawTextResult.idList_[i];
//         rawTextResult.numberOfDuplicatedDocs_[i] = dd_list.size();
//     }

    //TODO add other information



    if (!rawTextResult.error_.empty())
    {
        response_.addError(rawTextResult.error_);
        return false;
    }

    return true;
}

bool DocumentsSearchHandler::parse()
{
    std::vector<Parser*> parsers;
    std::vector<const Value*> values;

    SelectParser selectParser(indexSchema_);
    parsers.push_back(&selectParser);
    values.push_back(&request_[Keys::select]);

    SearchParser searchParser(indexSchema_);
    parsers.push_back(&searchParser);
    values.push_back(&request_[Keys::search]);

    FilteringParser filteringParser(indexSchema_,miningSchema_);
    parsers.push_back(&filteringParser);
    values.push_back(&request_[Keys::conditions]);

    CustomRankingParser customrRankingParser(indexSchema_);
    parsers.push_back(&customrRankingParser);
    values.push_back(&request_[Keys::custom_rank]);

    SortParser sortParser(indexSchema_);
    Value& customRankingValue = request_[Keys::custom_rank];
    if (customRankingValue.type() != Value::kNullType) {
        sortParser.validateCustomRank(true);
    }
    parsers.push_back(&sortParser);
    values.push_back(&request_[Keys::sort]);

    PageInfoParser pageInfoParser;
    parsers.push_back(&pageInfoParser);
    values.push_back(&request_.get());

    GroupingParser groupingParser;
    parsers.push_back(&groupingParser);
    values.push_back(&request_[Keys::group]);

    AttrParser attrParser;
    parsers.push_back(&attrParser);
    values.push_back(&request_[Keys::attr]);

    RangeParser rangeParser(indexSchema_);
    parsers.push_back(&rangeParser);
    values.push_back(&request_[Keys::range]);

    for (std::size_t i = 0; i < parsers.size(); ++i)
    {
        if (!parsers[i]->parse(*values[i]))
        {
            response_.addError(parsers[i]->errorMessage());
            return false;
        }
        response_.addWarning(parsers[i]->warningMessage());
    }

    parseOptions();

    // store parse result
    using std::swap;

    // selectParser
    swap(
        actionItem_.displayPropertyList_,
        selectParser.mutableProperties()
    );

    // searchParser
    swap(
        actionItem_.env_.queryString_,
        searchParser.mutableKeywords()
    );
    swap(
        actionItem_.env_.userID_,
        searchParser.mutableUserID()
    );
    swap(
        actionItem_.env_.taxonomyLabel_,
        searchParser.mutableTaxonomyLabel()
    );
    swap(
        actionItem_.env_.nameEntityItem_,
        searchParser.mutableNameEntityItem()
    );
    swap(
        actionItem_.env_.nameEntityType_,
        searchParser.mutableNameEntityType()
    );
    swap(
        actionItem_.groupParam_.groupLabels_,
        searchParser.mutableGroupLabels()
    );
    swap(
        actionItem_.groupParam_.attrLabels_,
        searchParser.mutableAttrLabels()
    );
    swap(
        actionItem_.searchPropertyList_,
        searchParser.mutableProperties()
    );
    actionItem_.languageAnalyzerInfo_ = searchParser.analyzerInfo();
    actionItem_.rankingType_ = searchParser.rankingModel();
    actionItem_.env_.isLogging_ = searchParser.logKeywords();

    // filteringParser
    swap(
        actionItem_.filteringList_,
        filteringParser.mutableFilteringRules()
    );

    // CustomRankingParser
    swap(
        actionItem_.customRanker_,
        customrRankingParser.getCustomRanker() // null after swap
    );
    actionItem_.strExp_ = actionItem_.customRanker_->getExpression();
    actionItem_.paramConstValueMap_ = actionItem_.customRanker_->getConstParamMap();
    actionItem_.paramPropertyValueMap_ = actionItem_.customRanker_->getPropertyParamMap();

    // orderArrayParser
    swap(
        actionItem_.sortPriorityList_,
        sortParser.mutableSortPropertyList()
    );

    // pageInfoParser
    actionItem_.pageInfo_.start_ = pageInfoParser.offset();
    actionItem_.pageInfo_.count_ = pageInfoParser.limit();

    // groupingParser
    swap(
        actionItem_.groupParam_.groupProps_,
        groupingParser.mutableGroupPropertyList()
    );

    // attrParser
    actionItem_.groupParam_.isAttrGroup_ = attrParser.attrResult();
    actionItem_.groupParam_.attrGroupNum_ = attrParser.attrTop();

    // rangeParser
    actionItem_.rangePropertyName_ = rangeParser.rangeProperty();

    std::string message;
    if (!actionItem_.groupParam_.checkParam(miningSchema_, message))
    {
        response_.addError(message);
        return false;
    }

    return true;
}

void DocumentsSearchHandler::parseOptions()
{
    actionItem_.removeDuplicatedDocs_ = asBool(request_[Keys::remove_duplicated_result]);
    returnAnalyzerResult_ = asBoolOr(request_[Keys::analyzer_result], false);
}

bool DocumentsSearchHandler::doSearch(
    KeywordSearchResult& searchResult
)
{
    if (!indexSearchService_->getSearchResult(actionItem_, searchResult))
    {
        response_.addError("Request Failed.");
        return false;
    }

    // Return analyzer result even when result validation fails.
    if (returnAnalyzerResult_ &&
            searchResult.analyzedQuery_.size() == actionItem_.searchPropertyList_.size())
    {
        std::string convertBuffer;
        Value& analyzerResult = response_[Keys::analyzer_result];
        for (std::size_t i = 0; i < searchResult.analyzedQuery_.size(); ++i)
        {
            searchResult.analyzedQuery_[i].convertString(
                convertBuffer, izenelib::util::UString::UTF_8
            );
            analyzerResult[actionItem_.searchPropertyList_[i]] = convertBuffer;
        }
    }

    if (!validateSearchResult(searchResult))
    {
        return false;
    }

// QueryCorrection should be called seperatedly
//    if (kRefinedQueryThreshold <=0 || searchResult.totalCount_ < std::size_t(kRefinedQueryThreshold))
//    {
//        izenelib::util::UString queryUString(
//            actionItem_.env_.queryString_,
//            izenelib::util::UString::UTF_8
//        );
//        QueryCorrectionSubmanager::getInstance().getRefinedQuery(
//            actionItem_.collectionName_,
//            queryUString,
//            actionItem_.refinedQueryString_
//        );
//    }

//    if (miningSearchService_)
//    {
//        if (!miningSearchService_->getSearchResult(searchResult))
//        {
//            response_.addWarning("Failed to get mining result.");
//            // render without mining result
//            return false;
//        }
//    }

    return true;
}

bool DocumentsSearchHandler::validateSearchResult(
    const KeywordSearchResult& siaResult
)
{
    if (!siaResult.error_.empty())
    {
        response_.addError(siaResult.error_);
        return false;
    }

    // TODO: SIA should ensure the result is valid.

    std::size_t displayPropertySize = actionItem_.displayPropertyList_.size();
    std::size_t summaryPropertySize = 0;

    typedef std::vector<DisplayProperty>::const_iterator iterator;
    for (iterator it = actionItem_.displayPropertyList_.begin(),
            itEnd = actionItem_.displayPropertyList_.end();
            it != itEnd; ++it)
    {
        if (it->isSummaryOn_)
        {
            summaryPropertySize++;
        }
    }
//     std::cout<<"**XX**"<<siaResult.fullTextOfDocumentInPage_.size()<<std::endl;
//     std::cout<<"**YY**"<<siaResult.snippetTextOfDocumentInPage_.size()<<std::endl;
//     std::cout<<"**ZZ**"<<siaResult.rawTextOfSummaryInPage_.size()<<std::endl;
//     std::cout<<"**AA**"<<siaResult.count_<<std::endl;
//     std::cout<<"**BB**"<<siaResult.start_<<std::endl;
//     std::cout<<"**CC**"<<siaResult.topKDocs_.size()<<std::endl;
    if ( !validateTextList(siaResult.fullTextOfDocumentInPage_,
                           siaResult.count_,
                           displayPropertySize) ||
            !validateTextList(siaResult.snippetTextOfDocumentInPage_,
                              siaResult.count_,
                              displayPropertySize) ||
            !validateTextList(siaResult.rawTextOfSummaryInPage_,
                              siaResult.count_,
                              summaryPropertySize))
    {
        response_.addError("Malformed search result.");
        return false;
    }

    return true;
}

bool DocumentsSearchHandler::validateTextList(
    const std::vector<std::vector<izenelib::util::UString> >& textList,
    std::size_t row,
    std::size_t column
)
{
    if (row > 0)
    {
        BOOST_ASSERT(textList.size() == column);
        if (textList.size() != column)
        {
            return false;
        }

        typedef std::vector<std::vector<izenelib::util::UString> > list_type;
        typedef list_type::const_iterator iterator;

        for (iterator it = textList.begin(); it != textList.end(); ++it)
        {
            BOOST_ASSERT(it->size() == row);

            if (it->size() != row)
            {
                return false;
            }
        }
    }

    return true;
}

// bool DocumentsSearchHandler::validateMiningResult(
//     const KeywordSearchResult& miaResult
// )
// {
//     if (!miaResult.error_.empty())
//     {
//         response_.addWarning(miaResult.error_);
//         return false;
//     }
//
//     return true;
// }

void DocumentsSearchHandler::renderDocuments(
    const RawTextResultFromMIA& rawTextResult
)
{
    renderer_.renderDocuments(
        actionItem_.displayPropertyList_,
        rawTextResult,
        response_[Keys::resources]
    );
}

void DocumentsSearchHandler::renderDocuments(
    const KeywordSearchResult& searchResult
)
{
    renderer_.renderDocuments(
        actionItem_.displayPropertyList_,
        searchResult,
        response_[Keys::resources]
    );
}

void DocumentsSearchHandler::renderMiningResult(
    const KeywordSearchResult& miaResult
)
{
    if (miningSearchService_)
    {
        renderer_.renderRelatedQueries(
            miaResult,
            response_[Keys::related_queries]
        );

//         renderer_.renderPopularQueries(
//             miaResult,
//             response_[Keys::popular_queries]
//         );
//
//         renderer_.renderRealTimeQueries(
//             miaResult,
//             response_[Keys::realtime_queries]
//         );

        renderer_.renderTaxonomy(
            miaResult,
            response_[Keys::taxonomy]
        );

        renderer_.renderNameEntity(
            miaResult,
            response_[Keys::name_entity]
        );

        renderer_.renderFaceted(
            miaResult,
            response_[Keys::faceted]
        );

        renderer_.renderGroup(
            miaResult,
            response_[Keys::group]
        );

        renderer_.renderAttr(
            miaResult,
            response_[Keys::attr]
        );
    }
}

void DocumentsSearchHandler::renderRangeResult(
    const KeywordSearchResult& searchResult
)
{
    if (!actionItem_.rangePropertyName_.empty())
    {
        Value& range = response_[Keys::range];
        range[Keys::min] = searchResult.propertyRange_.lowValue_;
        range[Keys::max] = searchResult.propertyRange_.highValue_;
    }
}

void DocumentsSearchHandler::renderRefinedQuery()
{
    if (!actionItem_.refinedQueryString_.empty())
    {
        std::string convertBuffer;
        actionItem_.refinedQueryString_.convertString(
            convertBuffer, izenelib::util::UString::UTF_8
        );
        response_[Keys::refined_query] = convertBuffer;
    }
}

void DocumentsSearchHandler::addAclFilters()
{
    PropertyConfig config;
    bool hasAclAllow = getPropertyConfig(indexSchema_, "ACL_ALLOW", config)
                       && config.isIndex()
                       && config.getIsFilter()
                       && config.getIsMultiValue();
    bool hasAclDeny = getPropertyConfig(indexSchema_, "ACL_DENY", config)
                      && config.isIndex()
                      && config.getIsFilter()
                      && config.getIsMultiValue();

    if (!hasAclAllow && !hasAclDeny)
    {
        return;
    }

    typedef std::string::value_type char_type;
    std::vector<std::string> tokens;
    if (!request_.aclTokens().empty())
    {
        boost::split(
            tokens,
            request_.aclTokens(),
            std::bind1st(std::equal_to<char_type>(), ','),
            boost::algorithm::token_compress_on
        );
    }

    // ACL_ALLOW
    if (hasAclAllow)
    {
        QueryFiltering::FilteringType filter;
        filter.operation_ = QueryFiltering::INCLUDE;
        filter.property_ = "ACL_ALLOW";

        filter.values_.assign(tokens.begin(), tokens.end());
        PropertyValue value(std::string("@@ALL@@"));
        filter.values_.push_back(value);

        izenelib::util::swapBack(actionItem_.filteringList_, filter);
    }

    // ACL_DENY
    if (hasAclDeny)
    {
        QueryFiltering::FilteringType filter;
        filter.operation_ = QueryFiltering::EXCLUDE;
        filter.property_ = "ACL_DENY";

        filter.values_.assign(tokens.begin(), tokens.end());

        izenelib::util::swapBack(actionItem_.filteringList_, filter);
    }
}

namespace detail
{
void DocumentsSearchKeywordsLogger::log(
    const KeywordSearchActionItem& actionItem,
    const KeywordSearchResult& searchResult,
    bool success
)
{
    // only log success queries
    if (!success || !actionItem.env_.isLogging_)
    {
        return;
    }
    // only log untruncated queries
    if (!actionItem.groupParam_.groupLabels_.empty() ||
        !actionItem.groupParam_.attrLabels_.empty() ||
        !actionItem.env_.taxonomyLabel_.empty() ||
        !actionItem.filteringList_.empty())
    {
        return;
    }

    static const std::string session = "SESSION NOT USED";

    // extract common information.
    boost::posix_time::ptime now =
        boost::posix_time::microsec_clock::local_time();

    boost::posix_time::time_duration duration = now - start_;

    izenelib::util::UString queryString(
        actionItem.env_.queryString_,
        izenelib::util::UString::UTF_8
    );

    // log for mining features, such as recent keywords
    UserQuery queryLog;
    queryLog.setQuery(actionItem.env_.queryString_);
    queryLog.setCollection(actionItem.collectionName_);
    queryLog.setHitDocsNum(searchResult.totalCount_);
    queryLog.setPageStart(actionItem.pageInfo_.start_);
    queryLog.setPageCount(actionItem.pageInfo_.count_);
    queryLog.setSessionId(session);
    queryLog.setTimeStamp(start_);
    queryLog.setDuration(duration);
    queryLog.save();
}
} // namespace detail

} // namespace sf1r
