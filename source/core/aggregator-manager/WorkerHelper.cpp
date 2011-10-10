#include "WorkerHelper.h"

#include <util/get.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

namespace sf1r
{

void assembleConjunction(std::vector<izenelib::util::UString> keywords, std::string& result)
{
    result.clear();
    int size = keywords.size();
    for(int i = 0; i < size; ++i)
    {
        std::string str;
        keywords[i].convertString(str, izenelib::util::UString::UTF_8);
        result += str;
        result += " ";
    }
}

void assembleDisjunction(std::vector<izenelib::util::UString> keywords, std::string& result)
{
    result.clear();
    int size = keywords.size();
    for(int i = 0; i < size; ++i)
    {
        std::string str;
        keywords[i].convertString(str, izenelib::util::UString::UTF_8);
        result += str;
        result += "|";
    }
    boost::trim_right_if(result,is_any_of("|"));	
}

bool buildQueryTree(SearchKeywordOperation&action, IndexBundleConfiguration& bundleConfig, std::string& btqError,  PersonalSearchInfo& personalSearchInfo)
{
    action.clear();
    KeywordSearchActionItem actionItem = action.actionItem_;

    // Build raw Query Tree
    UString::EncodingType encodingType =
        UString::convertEncodingTypeFromStringToEnum(
            action.actionItem_.env_.encodingType_.c_str() );
    UString queryUStr(action.actionItem_.env_.queryString_, encodingType);
    if ( !action.queryParser_.parseQuery( queryUStr, action.rawQueryTree_, action.unigramFlag_, action.hasUnigramProperty_ ) )
        return false;
    action.rawQueryTree_->print();

    // Build property query map
    bool applyLA = action.actionItem_.languageAnalyzerInfo_.applyLA_;

    std::vector<std::string>::const_iterator propertyIter = action.actionItem_.searchPropertyList_.begin();
    for (; propertyIter != action.actionItem_.searchPropertyList_.end(); propertyIter++)
    {
        // If there's already processed query, skip processing of this property..
        if ( action.queryTreeMap_.find( *propertyIter ) != action.queryTreeMap_.end()
                && action.propertyTermInfo_.find( *propertyIter ) != action.propertyTermInfo_.end() )
            continue;
        std::cout << "----------------->  processing query for Property : " << *propertyIter << std::endl;

        QueryTreePtr tmpQueryTree;
        if ( applyLA )
        {
            AnalysisInfo analysisInfo;
            std::string analysis, language;
            bundleConfig.getAnalysisInfo( *propertyIter, analysisInfo, analysis, language );

            PropertyConfig propertyConfig;
            propertyConfig.propertyName_ = *propertyIter;
            IndexBundleSchema::const_iterator it = bundleConfig.schema_.find(propertyConfig);
            if (it != bundleConfig.schema_.end() && (*it).isIndex() && (*it).getIsFilter())
            {
                analysisInfo.analyzerId_ = "la_sia";
                analysisInfo.tokenizerNameList_.insert("tok_divide");
                analysisInfo.tokenizerNameList_.insert("tok_unite");
            }

            bool isUnigramSearchMode = action.isUnigramSearchMode_;
            if ((*propertyIter).size() > 8 && (*propertyIter).rfind("_unigram") == ((*propertyIter).size()-8))
            {
                // only unigram terms indexed for property with unigram Alias, so
                // it will fail if use word segments as rank terms in unigram searching mode.
                isUnigramSearchMode = false;
            }

            action.actionItem_.env_.expandedQueryString_.clear();
            if ( !action.queryParser_.getAnalyzedQueryTree(
                        action.actionItem_.languageAnalyzerInfo_.synonymExtension_,
                        analysisInfo, queryUStr, tmpQueryTree, action.actionItem_.env_.expandedQueryString_,
                        action.unigramFlag_, action.hasUnigramProperty_, isUnigramSearchMode, personalSearchInfo))
                return false;

        } // end - if
        else // store raw query's info into it.
            tmpQueryTree = action.rawQueryTree_;

        tmpQueryTree->print();
        action.queryTreeMap_.insert( std::make_pair(*propertyIter,tmpQueryTree) );
        PropertyTermInfo ptInfo;
        tmpQueryTree->getPropertyTermInfo(ptInfo);
        action.propertyTermInfo_.insert( std::make_pair(*propertyIter,ptInfo) );

    } // end - for
    return true;
} // end - buildQueryTree()


}
