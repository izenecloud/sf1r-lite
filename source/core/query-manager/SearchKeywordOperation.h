///
/// @file SearchKeywordOperation.h
/// @brief header file of SearchKeywordOperation class
/// @author Dohyun Yun
/// @date 2010-06-22
///

#ifndef _SEARCHKEYWORDOPERATION_H_
#define _SEARCHKEYWORDOPERATION_H_

#include "ActionItem.h"
#include "QueryTree.h"
#include "QueryParser.h"

#include <common/PropertyTermInfo.h>
#include <common/type_defs.h>

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

namespace sf1r {

    class SearchKeywordOperation
    {
        public:
            SearchKeywordOperation(
                    const KeywordSearchActionItem& actionItem,
                    bool unigramFlag,
                    boost::shared_ptr<LAManager>& laManager,
                    boost::shared_ptr<izenelib::ir::idmanager::IDManager>& idManager);

            ~SearchKeywordOperation();

            ///
            /// @brief build query tree.
            /// @param[btqError] a string which contains error message during building query tree.
            /// @return true if success, or false.
            ///
            //bool buildQueryTree(std::string& btqError);

            ///
            /// @brief get query tree map.
            ///
            bool getQueryTree(const std::string& propertyName, QueryTreePtr& propertyQueryTree) const;

            ///
            /// @brief get query term id set
            ///
            bool getQueryTermIdSet(const std::string& propertyName, std::set<termid_t>& queryTermIdSet) const;

            ///
            /// @brief get query term id list
            ///
            bool getQueryTermIdList(const std::string& propertyName, std::vector<termid_t>& queryTermIdList) const;

            ///
            /// @brief get property term info map < property name - PropertyTermInfo class >
            ///
            const std::map<std::string,PropertyTermInfo>& getPropertyTermInfoMap() const;

            bool getRawQueryTermIdList( std::vector<termid_t>& rawQueryTermIdList ) const;

            bool noError() const { return noError_; }

            void clear();

        private:

            ///
            /// @brief set property query tree into queryTreeMap_
            ///
            bool setQueryTree(const std::string& propertyName, const QueryTreePtr& propertyQueryTree);

        public:
            KeywordSearchActionItem actionItem_;

            bool noError_;
            QueryTreePtr rawQueryTree_;
            boost::unordered_map<std::string,QueryTreePtr> queryTreeMap_;
            std::map<std::string,PropertyTermInfo> propertyTermInfo_;
            bool unigramFlag_; // wildcard type
            QueryParser queryParser_;

            bool hasUnigramProperty_;
            bool isUnigramSearchMode_;
    }; // end - class SearchKeywordOperation

} // end - namespace sf1vt

#endif // _SEARCHKEYWORDOPERATION_H_
