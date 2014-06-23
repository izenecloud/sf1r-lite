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
#include "GroupLabelPreProcessor.h"

#include <parsers/SelectParser.h>
#include <parsers/FilteringParser.h>
#include <parsers/GroupingParser.h>
#include <parsers/AttrParser.h>
#include <parsers/RangeParser.h>
#include <parsers/SearchParser.h>
#include <parsers/SortParser.h>
#include <parsers/CustomRankingParser.h>
#include <parsers/GeoLocationRankingParser.h>
#include <common/BundleSchemaHelpers.h>

#include <common/Keys.h>
#include <common/parsers/PageInfoParser.h>
#include <common/QueryNormalizer.h>

#include <mining-manager/MiningManager.h>

#include <log-manager/UserQuery.h>

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
        zambeziConfig_(collectionHandler.zambeziConfig_),
        actionItem_(),
        TOP_K_NUM(collectionHandler.indexSearchService_->getBundleConfig()->topKNum_),
        renderer_(miningSchema_, TOP_K_NUM)
{
    actionItem_.env_.encodingType_ = "UTF-8";
    actionItem_.env_.ipAddress_ = request.header()[Keys::remote_ip].getString();
    actionItem_.collectionName_ = asString(request[Keys::collection]);
}

void DocumentsSearchHandler::search()
{
    if (parse())
    {
        addAclFilters();
        KeywordSearchResult searchResult;
        preprocess(searchResult);

        searchResult.TOP_K_NUM = TOP_K_NUM;
        if (actionItem_.searchingMode_.mode_ == SearchingMode::SUFFIX_MATCH)
        {
            searchResult.TOP_K_NUM = actionItem_.searchingMode_.lucky_;
        }
        else if (actionItem_.searchingMode_.mode_ == SearchingMode::AD_INDEX)
        {
            std::vector<std::pair<std::string, std::string> >::iterator it;
            for(it = actionItem_.adSearchPropertyValue_.begin();
                    it != actionItem_.adSearchPropertyValue_.end(); it++ )
            {
                actionItem_.env_.queryString_ += (it->first + it->second);
            }
        }
        renderer_.setTopKNum(searchResult.TOP_K_NUM);

        int topKStart = actionItem_.pageInfo_.topKStart(TOP_K_NUM, IsTopKComesFromConfig(actionItem_));

        {
            // initialize before search to record start time.
            detail::DocumentsSearchKeywordsLogger keywordsLogger;
            if (doSearch(searchResult))
            {
                response_[Keys::total_count] = searchResult.totalCount_;

                std::size_t topKCount = searchResult.topKDocs_.size();
                if(IsTopKComesFromConfig(actionItem_))
                    if (topKStart + topKCount <= searchResult.totalCount_)
                        topKCount += topKStart;

                response_[Keys::top_k_count] = topKCount;

                renderDocuments(searchResult);
                renderMiningResult(searchResult);
                renderRangeResult(searchResult);
                renderCountResult(searchResult);
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
    }
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
        response_.addError("failed to get document");
        return false;
    }
    //add dd result
    rawTextResult.numberOfDuplicatedDocs_.resize(rawTextResult.idList_.size(), 0);
//     uint32_t pos1=0,pos2=0;
//     while (pos1<miaResult.topKDocs_.size() && pos<rawTextResult.idList_.size() )
//     {
//
//     }
//     for (uint32_t i=0;i<rawTextResult.idList_.size();i++)
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
    
    SearchParser searchParser(indexSchema_, zambeziConfig_);
    parsers.push_back(&searchParser);
    values.push_back(&request_[Keys::search]);

     for (std::size_t i = 0; i < 1; ++i)
    {
        if (!parsers[i]->parse(*values[i]))
        {
            response_.addError(parsers[i]->errorMessage());
            return false;
        }
        response_.addWarning(parsers[i]->warningMessage());
    }

    SelectParser selectParser(indexSchema_, zambeziConfig_, searchParser.searchingModeInfo().mode_);
    parsers.push_back(&selectParser);
    values.push_back(&request_[Keys::select]); 

    FilteringParser filteringParser(indexSchema_, miningSchema_); 
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
    Value& geoLocationValue = request_[Keys::geolocation];
    if (geoLocationValue.type() != Value::kNullType) {
        sortParser.validateGeoRank(true);
    }
    parsers.push_back(&sortParser);
    values.push_back(&request_[Keys::sort]);

    GeoLocationRankingParser geoLocationParser(indexSchema_);
    parsers.push_back(&geoLocationParser);
    values.push_back(&request_[Keys::geolocation]);

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

    for (std::size_t i = 1; i < parsers.size(); ++i)
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
        actionItem_.env_.querySource_,
        searchParser.mutableQuerySource()
    );
    swap(
        actionItem_.groupParam_.groupLabels_,
        searchParser.mutableGroupLabels()
    );
    swap(
        actionItem_.groupParam_.autoSelectLimits_,
        searchParser.mutableGroupLabelAutoSelectLimits()
    );
    swap(
        actionItem_.groupParam_.boostGroupLabels_,
        searchParser.mutableBoostGroupLabels()
    );
    swap(
        actionItem_.groupParam_.attrLabels_,
        searchParser.mutableAttrLabels()
    );
    swap(
        actionItem_.searchPropertyList_,
        searchParser.mutableProperties()
    );
    swap(
        actionItem_.counterList_,
        searchParser.mutableCounterList()
    );
    swap(
        actionItem_.adSearchPropertyValue_,
        searchParser.adSearch()
    );
    actionItem_.languageAnalyzerInfo_ = searchParser.analyzerInfo();
    actionItem_.rankingType_ = searchParser.rankingModel();
    actionItem_.searchingMode_ = searchParser.searchingModeInfo();
    actionItem_.env_.isLogging_ = searchParser.logKeywords();
    actionItem_.isRandomRank_ = searchParser.isRandomRank();
    actionItem_.requireRelatedQueries_ = searchParser.isRequireRelatedQueries();
    // filteringParser
    swap(
        actionItem_.filterTree_,
        filteringParser.mutableFilteringTreeRules()
    );

    // CustomRankingParser
    swap(
        actionItem_.customRanker_,
        customrRankingParser.getCustomRanker() // null after swap
    );
    actionItem_.strExp_ = actionItem_.customRanker_->getExpression();
    actionItem_.paramConstValueMap_ = actionItem_.customRanker_->getConstParamMap();
    actionItem_.paramPropertyValueMap_ = actionItem_.customRanker_->getPropertyParamMap();

    // GeoLocationRankingParser
    //swap(
    //    actionItem_.geoLocationRanker_,
    //    geoLocationRankingParser.getGeoLocationRanker() // null after swap
    //);
    actionItem_.geoLocation_ = geoLocationParser.getReference();
    actionItem_.geoLocationProperty_ = geoLocationParser.getPropertyName();

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
    actionItem_.groupParam_.searchMode_ = actionItem_.searchingMode_.mode_;

    // rangeParser
    actionItem_.rangePropertyName_ = rangeParser.rangeProperty();

    std::string message;
    if (!actionItem_.groupParam_.checkParam(miningSchema_, message))
    {
        response_.addError(message);
        return false;
    }

    if (!checkSuffixMatchParam(message))
    {
        response_.addError(message);
        return false;
    }

    return true;
}

bool DocumentsSearchHandler::checkSuffixMatchParam(std::string& message)
{
    if (actionItem_.searchingMode_.mode_ != SearchingMode::SUFFIX_MATCH
            || !actionItem_.searchingMode_.usefuzzy_)
        return true;

    std::vector<QueryFiltering::FilteringType> filteringRules;

    if (actionItem_.filterTree_.conditionsNodeList_.size() > 0)
    {
        message = "The Suffix Match search do not support nesting filter condition";
        return false;
    }

    for (unsigned int i = 0; i < actionItem_.filterTree_.conditionLeafList_.size(); ++i)
    {
        filteringRules.push_back(actionItem_.filterTree_.conditionLeafList_[i]);
    }

    /*
    for (int i = size -1 ; i >= 0; --i)
    {
        if (actionItem_.filteringTreeList_[i].isRelationNode_ == true)
        {
            if (i != 0)
            {
                message = "The Suffix Match search do not support nesting filter condition";
                return false;
            }
            else
                break;
        }
        filteringRules.push_back(actionItem_.filteringTreeList_[i].fitleringType_);
    }
    */

    const std::vector<QueryFiltering::FilteringType>& filter_param = filteringRules;
    const SuffixMatchConfig& suffixconfig = miningSchema_.suffixmatch_schema;
    for (size_t i = 0; i < filter_param.size(); ++i)
    {
        const QueryFiltering::FilteringType& filtertype = filter_param[i];
        if (std::find(suffixconfig.group_filter_properties.begin(),
                suffixconfig.group_filter_properties.end(),
                filtertype.property_) == suffixconfig.group_filter_properties.end() &&
            std::find(suffixconfig.attr_filter_properties.begin(),
                suffixconfig.attr_filter_properties.end(),
                filtertype.property_) == suffixconfig.attr_filter_properties.end())
        {
            bool finded = false;
            for (size_t j = 0; j < suffixconfig.num_filter_properties.size(); ++j)
            {
                if (filtertype.property_ == suffixconfig.num_filter_properties[j].property)
                {
                    finded = true;
                    break;
                }
            }
            if (!finded)
            {
                message = "The filter property : " + filtertype.property_ + " was not configured as FilterProperty in SuffixMatchConfig.";
                return false;
            }
        }
        PropertyConfig config;
        bool hasit = getPropertyConfig(indexSchema_, filtertype.property_, config);
        if (!hasit)
        {
            message = "The filter property:" + filtertype.property_ + " not found.";
            return false;
        }

        if (config.isNumericType())
        {
            if (filtertype.operation_ != QueryFiltering::GREATER_THAN_EQUAL &&
                filtertype.operation_ != QueryFiltering::GREATER_THAN &&
                filtertype.operation_ != QueryFiltering::LESS_THAN_EQUAL &&
                filtertype.operation_ != QueryFiltering::LESS_THAN &&
                filtertype.operation_ != QueryFiltering::EQUAL &&
                filtertype.operation_ != QueryFiltering::RANGE)
            {
                message = "The number filter type only support \"=\", \">=\", \"<=\", \"between\" operations.";
                return false;
            }
        }
        else
        {
            if (filtertype.operation_ != QueryFiltering::EQUAL &&
                filtertype.operation_ != QueryFiltering::GREATER_THAN_EQUAL &&
                filtertype.operation_ != QueryFiltering::GREATER_THAN &&
                filtertype.operation_ != QueryFiltering::LESS_THAN_EQUAL &&
                filtertype.operation_ != QueryFiltering::LESS_THAN &&
                filtertype.operation_ != QueryFiltering::RANGE &&
                filtertype.operation_ != QueryFiltering::PREFIX)
            {
                message = "The string filter type only support \"=\", \"in\" operations while using fuzzy searching.";
                return false;
            }
        }
    }
    const faceted::GroupParam& gp = actionItem_.groupParam_;
    faceted::GroupParam::GroupLabelMap::const_iterator cit = gp.groupLabels_.begin();
    while (cit != gp.groupLabels_.end())
    {
        if (std::find(suffixconfig.group_filter_properties.begin(),
                suffixconfig.group_filter_properties.end(),
                cit->first) == suffixconfig.group_filter_properties.end())
        {
            bool find_num_prop = false;
            for (size_t j = 0; j < suffixconfig.num_filter_properties.size(); ++j)
            {
                if (cit->first == suffixconfig.num_filter_properties[j].property)
                {
                    find_num_prop = true;
                    break;
                }
            }
            find_num_prop = find_num_prop || (std::find(suffixconfig.date_filter_properties.begin(),
                suffixconfig.date_filter_properties.end(), cit->first) != suffixconfig.date_filter_properties.end());
            if (!find_num_prop)
            {
                message = "The filter property : " + cit->first + " was not configured as group FilterProperty in SuffixMatchConfig.";
                return false;
            }
        }
        ++cit;
    }
    if (!gp.isAttrEmpty() && suffixconfig.attr_filter_properties.empty())
    {
        message = "The attribute filter was not configured in the SuffixMatchConfig.";
        return false;
    }
    return true;
}

void DocumentsSearchHandler::parseOptions()
{
    actionItem_.removeDuplicatedDocs_ = asBool(request_[Keys::remove_duplicated_result]);
    actionItem_.isAnalyzeResult_ = asBoolOr(request_[Keys::analyzer_result], false);
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
    if (actionItem_.isAnalyzeResult_)
    {
        std::string convertBuffer;
        searchResult.analyzedQuery_.convertString(
                convertBuffer, izenelib::util::UString::UTF_8
        );
        response_[Keys::analyzer_result] = convertBuffer;
    }

    if (!validateSearchResult(searchResult))
    {
        return false;
    }

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
    if (actionItem_.disableGetDocs_)
        return true;

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
    //LOG(INFO)<<"**XX**"<<siaResult.fullTextOfDocumentInPage_.size()<<std::endl;
    //LOG(INFO)<<"**YY**"<<siaResult.snippetTextOfDocumentInPage_.size()<<std::endl;
    //LOG(INFO)<<"**ZZ**"<<siaResult.rawTextOfSummaryInPage_.size()<<std::endl;
    //LOG(INFO)<<"**AA**"<<siaResult.count_<<std::endl;
    //LOG(INFO)<<"**BB**"<<siaResult.start_<<std::endl;
    //LOG(INFO)<<"**CC**"<<siaResult.topKDocs_.size()<<std::endl;
    //LOG(INFO)<<"**DD**"<<summaryPropertySize<<std::endl;
    //LOG(INFO)<<"**EE**"<<displayPropertySize<<std::endl;
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
    const std::vector<std::vector<PropertyValue::PropertyValueStrType> >& textList,
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

        typedef std::vector<std::vector<PropertyValue::PropertyValueStrType> > list_type;
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

        renderer_.renderGroup(
            miaResult,
            response_[Keys::group]
        );

        renderer_.renderAttr(
            miaResult,
            response_[Keys::attr]
        );

        renderer_.renderTopGroupLabel(
            miaResult,
            response_[Keys::top_group_label]
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

void DocumentsSearchHandler::renderCountResult(
    const KeywordSearchResult& searchResult
)
{
    std::map<std::string, unsigned>::const_iterator cit = searchResult.counterResults_.begin();
    for (; cit != searchResult.counterResults_.end(); ++cit)
    {
        response_[Keys::count][cit->first] = cit->second;
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

        for(size_t i = 0; i < tokens.size(); ++i)
        {
            filter.values_.push_back(PropertyValue(str_to_propstr(tokens[i])));
        }
        PropertyValue value(str_to_propstr("@@ALL@@"));
        filter.values_.push_back(value);

        izenelib::util::swapBack(actionItem_.filterTree_.conditionLeafList_, filter);
    }

    // ACL_DENY
    if (hasAclDeny)
    {
        QueryFiltering::FilteringType filter;
        filter.operation_ = QueryFiltering::EXCLUDE;
        filter.property_ = "ACL_DENY";

        for(size_t i = 0; i < tokens.size(); ++i)
        {
            filter.values_.push_back(PropertyValue(str_to_propstr(tokens[i])));
        }

        izenelib::util::swapBack(actionItem_.filterTree_.conditionLeafList_, filter);
    }
}

void DocumentsSearchHandler::preprocess(KeywordSearchResult& searchResult)
{
    QueryNormalizer::get()->normalize(actionItem_.env_.queryString_,
                                      actionItem_.env_.normalizedQueryString_);

    GroupLabelPreProcessor processor(miningSearchService_);
    processor.process(actionItem_, searchResult);
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
    //if (!actionItem.groupParam_.groupLabels_.empty() ||
        //!actionItem.groupParam_.attrLabels_.empty() ||
        //!actionItem.env_.taxonomyLabel_.empty() ||
        //!actionItem.filteringList_.empty())
    //{
        //return;
    //}

    static const std::string session = "SESSION NOT USED";

    // extract common information.
    boost::posix_time::ptime now =
        boost::posix_time::microsec_clock::local_time();

    boost::posix_time::time_duration duration = now - start_;

    izenelib::util::UString queryString(
        actionItem.env_.queryString_,
        izenelib::util::UString::UTF_8
    );
    for (unsigned int i = 0; i < queryString.length(); ++i)
    {
        if (queryString.isPunctuationChar(i))
        {
            return;
        }
    }
    // log for mining features, such as recent keywords
    UserQuery queryLog;

    queryLog.setQuery(actionItem.env_.queryString_);
    queryLog.setCollection(actionItem.collectionName_);
    queryLog.setHitDocsNum(searchResult.topKDocs_.size());
    queryLog.setPageStart(actionItem.pageInfo_.start_);
    queryLog.setPageCount(actionItem.pageInfo_.count_);
    queryLog.setSessionId(session);
    queryLog.setTimeStamp(start_);
    queryLog.setDuration(duration);
    queryLog.save();
}
} // namespace detail

} // namespace sf1r
