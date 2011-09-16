///
/// @brief  header file of Query Tree Structure
/// @author Dohyun Yun
/// @date   2010.06.16 (First Created)
/// - Log
///     - 2011.05.06 Extend tree node types for personalized search by Zhongxia Li
///

#ifndef _QUERY_TREE_H_
#define _QUERY_TREE_H_

#include <common/PropertyTermInfo.h>
#include <common/type_defs.h>
#include <util/ustring/UString.h>

#include <boost/shared_ptr.hpp>
#include <iostream>
#include <list>

namespace sf1r {

    class QueryTree
    {
        public:

            ///
            /// @brief  a variable which indicates type of current query node.
            ///
            enum QueryType { KEYWORD, RANK_KEYWORD, UNIGRAM_WILDCARD, TRIE_WILDCARD, AND, OR, NOT, EXACT, ORDER, NEARBY, ASTERISK, QUESTION_MARK, AND_PERSONAL, OR_PERSONAL, UNKNOWN } type_;

            ///
            /// @brief keyword string of current query node.
            ///
            std::string keyword_;

            ///
            /// @brief keyword ustring of current query node.
            ///
            izenelib::util::UString keywordUString_;

            ///
            /// @brief term id of keyword ustring.
            ///
            termid_t keywordId_;

            ///
            /// @brief this variable is used only for Nearby Type.  { A B }^distance_
            ///
            int distance_;

            ///
            /// @brief a list of chidren which current query node has.
            ///
            std::list<boost::shared_ptr<QueryTree> > children_;

            ///
            /// @brief termid set of current tree node.
            ///
            std::set<termid_t> queryTermIdSet_;

            ///
            /// @brief termid list of current tree node.
            ///
            std::vector<termid_t> queryTermIdList_;

            ///
            /// @brief term list of current tree node.
            ///
            std::vector<pair<termid_t, std::string> > queryTermInfoList_;

            ///
            /// @brief property term information of current node
            ///
            PropertyTermInfo propertyTermInfo_;

        public:
            QueryTree();
            QueryTree(QueryType type);

            ///
            /// @brief inserts child node inth current node.
            /// @param[childQueryTree] child query tree which is inserted into current node.
            ///
            void insertChild(const boost::shared_ptr<QueryTree>& childQueryTree);

            ///
            /// @brief build query term id set & property term info map.
            ///
            void postProcess(bool hasRankTerm = false);

            ///
            /// @brief gets query term id set.
            ///
            void getQueryTermIdSet(std::set<termid_t>& queryTermIdSet) const;

            ///
            /// @brief gets property term information object of current node.
            ///
            void getPropertyTermInfo(PropertyTermInfo& propertyTermInfo) const;

            ///
            /// @brief gets term id list of leaves in the query.
            ///
            void getLeafTermIdList(std::vector<termid_t>& leafTermIdList) const;

            ///
            /// @brief gets term info list of leaves in the query.
            ///
            void getQueryTermInfoList(std::vector<pair<termid_t, std::string> >& queryTermInfoList) const;

            ///
            /// @brief display content of current tree node from root to leaf.
            /// @param[out] ostream where content of tree is displayed.
            /// @param[tabLevel] This parameter indicates how many tabs are used before displaying current node.
            ///
            void print(std::ostream& out = std::cout, int tabLevel = 0) const;

        private:

            ///
            /// @brief fills property term info
            ///
            void recursivePreProcess(
                    std::set<termid_t>& queryTermIdSet,
                    std::vector<termid_t>& queryTermIdList,
                    std::vector<pair<termid_t, std::string> >& queryTermInfoList,
                    PropertyTermInfo& propertyTermInfo,
                    unsigned int& pos);

            void recursivePreProcessRankTerm(
                    std::set<termid_t>& queryTermIdSet,
                    std::vector<termid_t>& queryTermIdList,
                    std::vector<pair<termid_t, std::string> >& queryTermInfoList,
                    PropertyTermInfo& propertyTermInfo,
                    unsigned int& pos);

    }; // end - struct QueryTree

    typedef boost::shared_ptr<QueryTree> QueryTreePtr;
    typedef std::list<QueryTreePtr>::iterator QTIter;
    typedef std::list<QueryTreePtr>::const_iterator QTConstIter;

} // end - namespace sf1r

#endif // _QUERY_TREE_H_
