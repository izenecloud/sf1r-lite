#include "IndexBundleHelper.h"

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
    if ( !action.queryParser_.parseQuery( queryUStr, action.rawQueryTree_, action.unigramFlag_ ) )
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

            bool isUnigramSearchMode = action.isUnigramSearchMode_;
            if ((*propertyIter).size() > 8 && (*propertyIter).rfind("_unigram") == ((*propertyIter).size()-8))
            {
                // only unigram terms indexed for property with unigram Alias, so
                // it will fail if use word segments as rank terms in unigram searching mode.
                isUnigramSearchMode = false;
            }

            if ( !action.queryParser_.getAnalyzedQueryTree(
                        action.actionItem_.languageAnalyzerInfo_.synonymExtension_,
                        analysisInfo, queryUStr, tmpQueryTree, action.actionItem_.env_.expandedQueryString_,
                        action.unigramFlag_, isUnigramSearchMode, personalSearchInfo))
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

void split_string(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator )
{
    izenelib::util::UString str(szText);
    izenelib::util::UString sep(" ",encoding);
    sep[0] = Separator;
    size_t n = 0, nOld=0;
    while (n != izenelib::util::UString::npos)
    {
        n = str.find(sep,n);
        if (n != izenelib::util::UString::npos)
        {
            if (n != nOld)
                out.push_back(str.substr(nOld, n-nOld));
            n += sep.length();
            nOld = n;
        }
    }
    out.push_back(str.substr(nOld, str.length()-nOld));
}

void split_int(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator )
{
    izenelib::util::UString str(szText);
    izenelib::util::UString sep(" ",encoding);
    sep[0] = Separator;
    size_t n = 0, nOld=0;
    while (n != izenelib::util::UString::npos)
    {
        n = str.find(sep,n);
        if (n != izenelib::util::UString::npos)
        {
            if (n != nOld)
            {
                int64_t value = 0;
                try
                {
                    value = boost::lexical_cast< int64_t >( str.substr(nOld, n-nOld) );
                    out.push_back(value);
                }
                catch( const boost::bad_lexical_cast & )
                {}
            }
            n += sep.length();
            nOld = n;
        }
    }

    int64_t value = 0;
    try
    {
        value = boost::lexical_cast< int64_t >( str.substr(nOld, str.length()-nOld) );
        out.push_back(value);
    }
    catch( const boost::bad_lexical_cast & )
    {}
}

void split_float(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator )
{
    izenelib::util::UString str(szText);
    izenelib::util::UString sep(" ",encoding);
    sep[0] = Separator;
    size_t n = 0, nOld=0;
    while (n != izenelib::util::UString::npos)
    {
        n = str.find(sep,n);
        if (n != izenelib::util::UString::npos)
        {
            if (n != nOld)
            {
                float value = 0;
                try
                {
                    value = boost::lexical_cast< float >( str.substr(nOld, n-nOld) );
                    out.push_back(value);
                }
                catch( const boost::bad_lexical_cast & )
                {}
            }
            n += sep.length();
            nOld = n;
        }
    }

    float value = 0;
    try
    {
        value = boost::lexical_cast< float >( str.substr(nOld, str.length()-nOld) );
        out.push_back(value);
    }
    catch( const boost::bad_lexical_cast & )
    {}
}

}
