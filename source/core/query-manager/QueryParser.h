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

#include <boost/thread/once.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>

namespace sf1r {

    using namespace BOOST_SPIRIT_CLASSIC_NS;
    class QueryParser : public grammar<QueryParser>
    {
        public:
            typedef uint16_t const* iterator_t;
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
                        >> !( (root_node_d[chlit<uint16_t>('&')] >> boolQuery) | (root_node_d[chlit<uint16_t>('|')] >> boolQuery) );
                    notQuery = root_node_d[chlit<uint16_t>('!')] >> (stringQuery | priorQuery | exactQuery);

                    stringQuery = leaf_node_d[ lexeme_d[+(~chset<uint16_t>(operUStr_.c_str()))] ];

                    exactQuery = root_node_d[chlit<uint16_t>('\"')] >> exactString >> no_node_d[chlit<uint16_t>('\"')];
                    exactString = leaf_node_d[ lexeme_d[+(~chlit<uint16_t>('\"'))] ];

                    orderedQuery = root_node_d[chlit<uint16_t>('[')] >> orderedString >> no_node_d[chlit<uint16_t>(']')];
                    orderedString = leaf_node_d[ lexeme_d[+(~chlit<uint16_t>(']'))] ];

                    nearbyQuery = root_node_d[chlit<uint16_t>('{')] >> nearbyString >> no_node_d[chlit<uint16_t>('}')]
                        >> !( no_node_d[chlit<uint16_t>('^')] >> uint_p );
                    nearbyString = leaf_node_d[ lexeme_d[+(~chlit<uint16_t>('}'))] ];

                    priorQuery = inner_node_d[chlit<uint16_t>('(') >> boolQuery >> chlit<uint16_t>(')')];

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
            static izenelib::util::UString operUStr_;
            static izenelib::util::UString escOperUStr_;

            static std::vector<std::pair<izenelib::util::UString , izenelib::util::UString> > operEncodeDic_; // < "\^" , "::$OP_UP$::" >
            static std::vector<std::pair<izenelib::util::UString , izenelib::util::UString> > operDecodeDic_; // < "::$OP_UP$::" , "^" >

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
            /// @param[queryUStr] input and output of this interface.
            ///
            static void processEscapeOperator(izenelib::util::UString& queryUStr);

            ///
            /// @brief recorver regestered term to operators so that it can be applied as a original keyword.
            /// @details Example :    ::$OP_UP$:: => ^
            /// @param[queryUStr] input and output of this interface.
            ///
            static void recoverEscapeOperator(izenelib::util::UString& queryUStr);

            ///
            /// @brief add \ to operators which is used in the recursive analyzing.
            /// @details Example : ^ => \^
            ///
            static void addEscapeCharToOperator(izenelib::util::UString& queryUStr);

            ///
            /// @brief remove \ from operators.
            /// @details Example : \^ => ^
            ///
            static void removeEscapeChar(izenelib::util::UString& queryUStr);

            ///
            /// @brief normalizes query string before parsing. It removes or adds space.
            /// @details
            ///     - Step 1 : Remove continuous space and the space before and after |.
            ///         - (  hello   |    world  ) -> (hello|world)
            ///     - Step 2 : Remove or add space after or behind of specific operator.
            ///         - ( Hello World) -> (Hello World)
            ///         - {test case } ^ 12(month case) -> {test case}^12 (month case)
            ///         - (bracket close)(open space) -> (bracket close) (open space)
            /// @param[queryUStr] source query string.
            /// @param[normUStr] normalized query string.
            /// @return true if success or false.
            ///
            static void normalizeQuery(const izenelib::util::UString& queryUStr, izenelib::util::UString& normUStr, bool hasUnigramProperty);

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
                    izenelib::util::UString& queryUStr,
                    QueryTreePtr& queryTree,
                    bool unigramFlag,
                    bool hasUnigramProperty,
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
                    izenelib::util::UString& rawUStr,
                    QueryTreePtr& analyzedQueryTree,
                    std::string& expandedQueryString,
                    bool unigramFlag,
                    bool hasUnigramProperty,
                    bool isUnigramSearchMode,
                    PersonalSearchInfo& personalSearchInfo);

            /// @brief for testing
            bool getAnalyzedQueryTree(
                                bool synonymExtension,
                                const AnalysisInfo& analysisInfo,
                                izenelib::util::UString& rawUStr,
                                QueryTreePtr& analyzedQueryTree,
                                bool unigramFlag,
                                bool hasUnigramProperty)
            {
                std::string expandedQueryString;
                PersonalSearchInfo personalSearchInfo;
                personalSearchInfo.enabled = false;
                return getAnalyzedQueryTree(synonymExtension, analysisInfo, rawUStr, analyzedQueryTree, expandedQueryString,
                        unigramFlag, hasUnigramProperty, false, personalSearchInfo);
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
            /// @brief extends query tree with rank keywords
            ///
            bool extendRankKeywords(QueryTreePtr& queryTree, la::TermList& termList);

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
                    bool isUnigramSearchMode,
                    PersonalSearchInfo& personalSearchInfo,
                    std::string& expandedQueryString);

            static void initOnlyOnceCore();

            bool getQueryTree(iter_t const& i, QueryTreePtr& queryTree, bool unigramFlag);
            bool processKeywordAssignQuery( iter_t const& i, QueryTreePtr& queryTree, bool unigramFlag);
            bool processExactQuery( iter_t const& i, QueryTreePtr& queryTree);
            bool processBracketQuery( iter_t const& i, QueryTree::QueryType queryType, QueryTreePtr& queryTree, bool unigramFlag);
            bool tokenizeBracketQuery(
                    const izenelib::util::UString& queryUStr,
                    const AnalysisInfo& analysisInfo,
                    QueryTree::QueryType queryType,
                    QueryTreePtr& queryTree,
                    int distance = 0 // used only for nearby
                    );
            bool processBoolQuery(iter_t const& i, QueryTreePtr& queryTree, bool unigramFlag);
            bool processChildTree(iter_t const& i, QueryTreePtr& queryTree, bool unigramFlag);
            static bool processReplaceAll(izenelib::util::UString& queryUStr, std::vector<std::pair<izenelib::util::UString,izenelib::util::UString> >& dic);

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
