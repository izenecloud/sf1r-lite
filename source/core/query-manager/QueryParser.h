///
/// @brief  header file of Query Parser
/// @author Dohyun Yun
/// @date   2010.06.16 (First Created)
///

#ifndef _QUERY_PARSER_H_
#define _QUERY_PARSER_H_

#define BOOST_SPIRIT_THREADSAFE
//#define BOOST_SPIRIT_DEBUG

#include "QueryTree.h"
#include "LAEXInfo.h"
#include "QMCommonFunc.h"

#include <la-manager/LAManager.h>
#include <search-manager/PersonalizedSearchInfo.h>
#include <ir/id_manager/IDManager.h>

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_chset.hpp>

#include <boost/unordered_map.hpp>
#include <boost/thread/once.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>

namespace sf1r {

    using namespace BOOST_SPIRIT_CLASSIC_NS;
    class QueryParser : public grammar<QueryParser>
    {
        public:
            typedef char const* iterator_t;
            typedef tree_match<iterator_t> parse_tree_match_t;
            typedef parse_tree_match_t::tree_iterator iter_t;

            static const int stringQueryID      = 100;
            static const int boolQueryID        = 200;
            static const int notQueryID         = 201;
            static const int exactQueryID       = 300;
            static const int orderedQueryID     = 400;
            static const int nearbyQueryID      = 500;

            template <typename ScannerT> struct definition
            {
                definition(const QueryParser&) // Rule definition area
                {

                    rootQuery = root_node_d[eps_p] >> +boolQuery;

                    boolQuery = (stringQuery | notQuery | priorQuery | exactQuery | orderedQuery | nearbyQuery)
                        >> !( (root_node_d[ch_p(' ')] >> boolQuery) | (root_node_d[ch_p('|')] >> boolQuery) );
                    notQuery = root_node_d[ch_p('!')] >> (stringQuery | priorQuery);

                    stringQuery = leaf_node_d[ lexeme_d[+(~chset_p(operStr_.c_str()))] ];

                    exactQuery = root_node_d[ch_p('\"')] >> exactString >> no_node_d[ch_p('\"')];
                    exactString = leaf_node_d[ lexeme_d[+(~ch_p('\"'))] ];

                    orderedQuery = root_node_d[ch_p('[')] >> orderedString >> no_node_d[ch_p(']')];
                    orderedString = leaf_node_d[ lexeme_d[+(~ch_p(']'))] ];

                    nearbyQuery = root_node_d[ch_p('{')] >> nearbyString >> no_node_d[ch_p('}')]
                        >> !( no_node_d[ch_p('^')] >> uint_p );
                    nearbyString = leaf_node_d[ lexeme_d[+(~ch_p('}'))] ];

                    priorQuery = inner_node_d[ch_p('(') >> boolQuery >> ch_p(')')];

                } // end - definition()

                rule<ScannerT> rootQuery, priorQuery, exactString, orderedString, nearbyString;
                rule<ScannerT, parser_context<>, parser_tag<stringQueryID> > stringQuery;
                rule<ScannerT, parser_context<>, parser_tag<exactQueryID> > exactQuery;
                rule<ScannerT, parser_context<>, parser_tag<orderedQueryID> > orderedQuery;
                rule<ScannerT, parser_context<>, parser_tag<nearbyQueryID> > nearbyQuery;
                rule<ScannerT, parser_context<>, parser_tag<boolQueryID> > boolQuery;
                rule<ScannerT, parser_context<>, parser_tag<notQueryID> > notQuery;
                rule<ScannerT> const& start() const { return rootQuery; } // Return Start rule of rule in
            }; // end - definition

        private: // private member variables
            static std::string operStr_;
            static std::string escOperStr_;

            static boost::unordered_map<std::string , std::string> operEncodeDic_; // < "\^" , "::$OP_UP$::" >
            static boost::unordered_map<std::string , std::string> operDecodeDic_; // < "::$OP_UP$::" , "^" >

            static boost::unordered_map<char, bool> openBracket_;  // <bracket char , if close bracket>
            static boost::unordered_map<char, bool> closeBracket_;  // <bracket char , if close bracket>

            boost::shared_ptr<LAManager> laManager_;
            boost::shared_ptr<izenelib::ir::idmanager::IDManager> idManager_;

        public: // static interface

            QueryParser(
                    boost::shared_ptr<LAManager>& laManager, 
                    boost::shared_ptr<izenelib::ir::idmanager::IDManager>& idManager );

            ///
            /// @brief initialization function of QueryParser. This must be executed before using any other interfaces.
            ///
            static void initOnlyOnce();

            ///
            /// @brief replaces escaped operators to the regestered term.
            /// @details Example :    \^ => ::$OP_UP$::
            /// @param[queryString] input and output of this interface.
            ///
            static void processEscapeOperator(std::string& queryString);

            ///
            /// @brief recorver regestered term to operators so that it can be applied as a original keyword.
            /// @details Example :    ::$OP_UP$:: => ^
            /// @param[queryString] input and output of this interface.
            ///
            static void recoverEscapeOperator(std::string& queryString);

            ///
            /// @brief add \ to operators which is used in the recursive analyzing.
            /// @details Example : ^ => \^
            ///
            static void addEscapeCharToOperator(std::string& queryString);

            ///
            /// @brief remove \ from operators.
            /// @details Example : \^ => ^
            ///
            static void removeEscapeChar(std::string& queryString);

            ///
            /// @brief normalizes query string before parsing. It removes or adds space.
            /// @details
            ///     - Step 1 : Remove continuous space and the space before and after |.
            ///         - (  hello   |    world  ) -> (hello|world)
            ///     - Step 2 : Remove or add space after or behind of specific operator.
            ///         - ( Hello World) -> (Hello World)
            ///         - {test case } ^ 12(month case) -> {test case}^12 (month case)
            ///         - (bracket close)(open space) -> (bracket close) (open space)
            /// @param[queryString] source query string.
            /// @param[normString] normalized query string.
            /// @return true if success or false.
            ///
            static void normalizeQuery(const std::string& queryString, std::string& normString);

            ///
            /// @brief parses query and generates QueryTree.
            /// @param[queryUString] ustring of query.
            /// @param[queryTree] output of this interface which contains tree structure of query.
            /// @param[unigramFlag] a flag if parsing rule of "orderby unigram wildcard search" is used for wildcard type query.
            /// @param[removeChineseSpace] a flag to remove redundant space in chinese query. This value should be "false" 
            ///                            if analyzed query is used as a parameter of parseQuery().
            /// @return true if success, or false.
            ///
            bool parseQuery( 
                    const izenelib::util::UString& queryUStr, 
                    QueryTreePtr& queryTree, 
                    bool unigramFlag,
                    bool removeChineseSpace = true );

            ///
            /// @brief applies language analyzing process in the query tree.
            /// @param[synonymExtension] a flag of synonym extension
            /// @param[analysisInfo] analysis information.
            /// @param[rawUStr] raw ustring query.
            /// @param[analyzedQueryTree] the output which means la extended query tree.
            /// @return true if success, or false.
            ///
            bool getAnalyzedQueryTree(
                    bool synonymExtension,
                    const AnalysisInfo& analysisInfo,
                    const izenelib::util::UString& rawUStr,
                    QueryTreePtr& analyzedQueryTree,
                    bool unigramFlag,
                    PersonalSearchInfo& personalSearchInfo);

            bool getAnalyzedQueryTree(
                                bool synonymExtension,
                                const AnalysisInfo& analysisInfo,
                                const izenelib::util::UString& rawUStr,
                                QueryTreePtr& analyzedQueryTree,
                                bool unigramFlag)
            {
                PersonalSearchInfo personalSearchInfo;
                personalSearchInfo.enabled = false;
                return getAnalyzedQueryTree(synonymExtension, analysisInfo, rawUStr, analyzedQueryTree, unigramFlag, personalSearchInfo);
            }

        private:

            ///
            /// @brief extends wildcard node in tree using unigram.
            /// @param[queryTree] input and output query tree
            /// @return true if success, or false.
            ///
            bool extendUnigramWildcardTree(QueryTreePtr& queryTree);


            ///
            /// @brief extends wildcard node in tree using trie.
            /// @param[queryTree] input and output query tree
            /// @return true if success, or false.
            ///
            bool extendTrieWildcardTree(QueryTreePtr& queryTree);

            ///
            /// @brief extends query tree with personal search information
            /// @param[queryTree] keyword query tree
            /// @param[userTermList] user profile info
            /// @return true if success, or false.
            ///
            bool extendPersonlSearchTree(QueryTreePtr& queryTree, std::vector<UString>& userTermList);

            ///
            /// @brief extends keyword query tree by using language analysis.
            /// @param[queryTree] query tree which is the target of extension.
            /// @param[laInfo] language analysis information which is used to extend keyword.
            /// @return true if success, or false.
            ///
            bool recursiveQueryTreeExtension(
                    QueryTreePtr& queryTree, 
                    const LAEXInfo& laInfo,
                    PersonalSearchInfo& personalSearchInfo);

            static void initOnlyOnceCore();

            bool getQueryTree(iter_t const& i, QueryTreePtr& queryTree, bool unigramFlag);
            bool processKeywordAssignQuery( iter_t const& i, QueryTreePtr& queryTree, bool unigramFlag);
            bool processExactQuery( iter_t const& i, QueryTreePtr& queryTree);
            bool processBracketQuery( iter_t const& i, QueryTree::QueryType queryType, QueryTreePtr& queryTree, bool unigramFlag);
            bool tokenizeBracketQuery(
                    const std::string& queryStr, 
                    const AnalysisInfo& analysisInfo, 
                    QueryTree::QueryType queryType, 
                    QueryTreePtr& queryTree,
                    int distance = 0 // used only for nearby
                    );
            bool processBoolQuery(iter_t const& i, QueryTreePtr& queryTree, bool unigramFlag);
            bool processChildTree(iter_t const& i, QueryTreePtr& queryTree, bool unigramFlag);
            static bool processReplaceAll(std::string& queryString, boost::unordered_map<std::string,std::string>& dic);

            ///
            /// @brief set utf8 string into keyword_, keywordId_ and keyowrdUString_ of QueryTree
            /// @return true if no restriction occurs, or false.
            ///
            bool setKeyword(QueryTreePtr& queryTree, const std::string& utf8Str);

            ///
            /// @see setKeyword(QueryTreePtr& queryTree, std::string& utf8Str)
            ///
            bool setKeyword(QueryTreePtr& queryTree, const izenelib::util::UString& uStr);

    }; // end - struct QueryParser.h

} // end - namespace sf1r

#endif // _QUERY_PARSER_H_
