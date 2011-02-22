///
/// @file   SearchKeywordOperation.cpp
/// @brief  The source file of search keyword operation class.
/// @author Dohyun Yun
/// @date   2010.06.22
/// @details
/// - Log
///

#include "SearchKeywordOperation.h"
#include "QueryParser.h"

namespace sf1r {

    using izenelib::util::UString;

    SearchKeywordOperation::SearchKeywordOperation(
            const KeywordSearchActionItem& actionItem,
            const CollectionMeta& collectionMeta,
            boost::shared_ptr<LAManager>& laManager,
            boost::shared_ptr<izenelib::ir::idmanager::IDManager>& idManager):
        actionItem_(actionItem),
        noError_(true),
        collectionMeta_(collectionMeta),
        unigramFlag_(collectionMeta_.isUnigramWildcard()),
        queryParser_(laManager, idManager)
    {
    } // end - SearchKeywordOperation()

    SearchKeywordOperation::~SearchKeywordOperation()
    {
    } // end - ~SearchKeywordOperation()

    bool SearchKeywordOperation::buildQueryTree(std::string& btqError)
    {
        clear();

        // Build raw Query Tree
        UString::EncodingType encodingType =
            UString::convertEncodingTypeFromStringToEnum(
                    actionItem_.env_.encodingType_.c_str() );
        UString queryUStr(actionItem_.env_.queryString_, encodingType);
        if ( !queryParser_.parseQuery( queryUStr, rawQueryTree_, unigramFlag_ ) )
            return false;
rawQueryTree_->print();

        // Build property query map
        bool applyLA = actionItem_.languageAnalyzerInfo_.applyLA_;

        std::vector<std::string>::const_iterator propertyIter = actionItem_.searchPropertyList_.begin();
        for(; propertyIter != actionItem_.searchPropertyList_.end(); propertyIter++)
        {
            // If there's already processed query, skip processing of this property..
            if ( queryTreeMap_.find( *propertyIter ) != queryTreeMap_.end()
                    && propertyTermInfo_.find( *propertyIter ) != propertyTermInfo_.end() )
                continue;

            QueryTreePtr tmpQueryTree;
            if( applyLA )
            {
                AnalysisInfo analysisInfo;
                std::string analysis, language;
                collectionMeta_.getAnalysisInfo( *propertyIter, analysisInfo, analysis, language );

                if ( !queryParser_.getAnalyzedQueryTree(
                            actionItem_.languageAnalyzerInfo_.synonymExtension_,
                            analysisInfo, queryUStr, tmpQueryTree, unigramFlag_ ))
                    return false;
            } // end - if
            else // store raw query's info into it.
                tmpQueryTree = rawQueryTree_;
std::cout << "Property " << *propertyIter << std::endl;
tmpQueryTree->print();
            queryTreeMap_.insert( std::make_pair(*propertyIter,tmpQueryTree) );
            PropertyTermInfo ptInfo; tmpQueryTree->getPropertyTermInfo(ptInfo);
            propertyTermInfo_.insert( std::make_pair(*propertyIter,ptInfo) );

        } // end - for
        return true;
    } // end - buildQueryTree()

    bool SearchKeywordOperation::getQueryTree(const std::string& propertyName, QueryTreePtr& propertyQueryTree) const
    {
        boost::unordered_map<std::string,QueryTreePtr>::const_iterator iter = queryTreeMap_.find( propertyName );
        if ( iter == queryTreeMap_.end() ) return false;
        propertyQueryTree = iter->second;

        return true;
    } // end - getQueryTreeMap()

    bool SearchKeywordOperation::setQueryTree(const std::string& propertyName, const QueryTreePtr& propertyQueryTree)
    {
        boost::unordered_map<std::string,QueryTreePtr>::const_iterator iter = queryTreeMap_.find( propertyName );
        if ( iter != queryTreeMap_.end() ) return false;
        queryTreeMap_.insert( std::make_pair(propertyName,propertyQueryTree) );
        return true;
    } // end - setQueryTree()

    bool SearchKeywordOperation::getQueryTermIdSet(const std::string& propertyName, std::set<termid_t>& queryTermIdSet) const
    {
        boost::unordered_map<std::string,QueryTreePtr>::const_iterator iter = queryTreeMap_.find( propertyName );
        if ( iter == queryTreeMap_.end() ) return false;
        iter->second->getQueryTermIdSet(queryTermIdSet);
        return true;
    } // end - getQueryTermIdSetMap()

    bool SearchKeywordOperation::getQueryTermIdList(const std::string& propertyName, std::vector<termid_t>& queryTermIdList) const
    {
        boost::unordered_map<std::string,QueryTreePtr>::const_iterator iter = queryTreeMap_.find( propertyName );
        if ( iter == queryTreeMap_.end() ) return false;
        iter->second->getLeafTermIdList(queryTermIdList);
        return true;
    } // end - getQueryTermIdSetMap()

    const std::map<std::string,PropertyTermInfo>& SearchKeywordOperation::getPropertyTermInfoMap() const
    {
        return propertyTermInfo_;
    } // end - getPropertyTermInfoMap()

    bool SearchKeywordOperation::getRawQueryTermIdList( std::vector<termid_t>& rawQueryTermIdList ) const
    {
        if ( !rawQueryTree_ )
            return false;
        rawQueryTree_->getLeafTermIdList( rawQueryTermIdList );
        return true;
    } // end - getRawQueryTermIdList()

    void SearchKeywordOperation::clear()
    {
        noError_ = true;
        queryTreeMap_.clear();
        propertyTermInfo_.clear();
    }
} // end - sf1r
