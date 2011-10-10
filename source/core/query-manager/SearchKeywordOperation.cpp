///
/// @file   SearchKeywordOperation.cpp
/// @brief  The source file of search keyword operation class.
/// @author Dohyun Yun
/// @date   2010.06.22
/// @details
/// - Log
///

#include "SearchKeywordOperation.h"

namespace sf1r {

    using izenelib::util::UString;

    SearchKeywordOperation::SearchKeywordOperation(
            const KeywordSearchActionItem& actionItem,
            bool unigramFlag,
            boost::shared_ptr<LAManager>& laManager,
            boost::shared_ptr<izenelib::ir::idmanager::IDManager>& idManager):
        actionItem_(actionItem),
        noError_(true),
        unigramFlag_(unigramFlag),
        queryParser_(laManager, idManager),
        hasUnigramProperty_(true),
        isUnigramSearchMode_(false)
    {
    } // end - SearchKeywordOperation()

    SearchKeywordOperation::~SearchKeywordOperation()
    {
    } // end - ~SearchKeywordOperation()

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
    } // end - getQueryTermIdSet()

    bool SearchKeywordOperation::getQueryTermIdList(const std::string& propertyName, std::vector<termid_t>& queryTermIdList) const
    {
        boost::unordered_map<std::string,QueryTreePtr>::const_iterator iter = queryTreeMap_.find( propertyName );
        if ( iter == queryTreeMap_.end() ) return false;
        iter->second->getLeafTermIdList(queryTermIdList);
        return true;
    } // end - getQueryTermIdList()

    bool SearchKeywordOperation::getQueryTermInfoList(const std::string& propertyName, std::vector<pair<termid_t, std::string> >& queryTermInfoList) const
    {
         boost::unordered_map<std::string,QueryTreePtr>::const_iterator iter = queryTreeMap_.find( propertyName );
         if ( iter == queryTreeMap_.end() ) return false;
         iter->second->getQueryTermInfoList(queryTermInfoList);
         return true;
    } // end - getQueryTermList()

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
