///
/// @brief  source file of Query Parser
/// @author Dohyun Yun
/// @date   2010.06.16 (First Created)
///

#include "QueryManager.h"
#include "QueryParser.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <algorithm>

namespace sf1r
{
    using namespace std;
    using izenelib::util::UString;

    // Declare static variables
    string QueryParser::operStr_;
    string QueryParser::escOperStr_;
    boost::unordered_map<string , string> QueryParser::operEncodeDic_;
    boost::unordered_map<string , string> QueryParser::operDecodeDic_;

    QueryParser::QueryParser(
            boost::shared_ptr<LAManager>& laManager,
            boost::shared_ptr<izenelib::ir::idmanager::IDManager>& idManager ):
        grammar<QueryParser>(), laManager_(laManager), idManager_(idManager)
    {
    } // end - QueryParser()

    void QueryParser::initOnlyOnce()
    {
        static boost::once_flag once = BOOST_ONCE_INIT;
        boost::call_once(&initOnlyOnceCore, once);
    } // end - initOnlyOnce()

    void QueryParser::initOnlyOnceCore()
    {
        operStr_.assign("&|!(){}[]^\"");
        escOperStr_ = operStr_; escOperStr_ += '\\';

        operEncodeDic_.insert( make_pair( "\\\\"    , "::$OP_SL$::" ) ); // "\\" : Operator Slash
        operEncodeDic_.insert( make_pair( "\\&"     , "::$OP_AN$::" ) ); // "\ " : Operator AND
        operEncodeDic_.insert( make_pair( "\\|"     , "::$OP_OR$::" ) ); // "\|" : Operator OR
        operEncodeDic_.insert( make_pair( "\\!"     , "::$OP_NT$::" ) ); // "\!" : Operator NOT
        operEncodeDic_.insert( make_pair( "\\("     , "::$OP_BO$::" ) ); // "\(" : Operator Opening Bracket
        operEncodeDic_.insert( make_pair( "\\)"     , "::$OP_BC$::" ) ); // "\)" : Operator Closing Bracket
        operEncodeDic_.insert( make_pair( "\\["     , "::$OP_OO$::" ) ); // "\[" : Operator Opening Orderby Bracket
        operEncodeDic_.insert( make_pair( "\\]"     , "::$OP_OC$::" ) ); // "\]" : Operator Closing Orderby Bracket
        operEncodeDic_.insert( make_pair( "\\{"     , "::$OP_NO$::" ) ); // "\{" : Operator Opening Nearby Bracket
        operEncodeDic_.insert( make_pair( "\\}"     , "::$OP_NC$::" ) ); // "\}" : Operator Closing Nearby Bracket
        operEncodeDic_.insert( make_pair( "\\^"     , "::$OP_UP$::" ) ); // "\^" : Operator Upper Arrow
        operEncodeDic_.insert( make_pair( "\\\""    , "::$OP_EX$::" ) ); // "\"" : Operator Exact Bracket

        operDecodeDic_.insert( make_pair( "::$OP_SL$::" , "\\" ) ); // "\" : Operator Slash
        operDecodeDic_.insert( make_pair( "::$OP_AN$::" , "&"  ) ); // " " : Operator AND
        operDecodeDic_.insert( make_pair( "::$OP_OR$::" , "|"  ) ); // "|" : Operator OR
        operDecodeDic_.insert( make_pair( "::$OP_NT$::" , "!"  ) ); // "!" : Operator NOT
        operDecodeDic_.insert( make_pair( "::$OP_BO$::" , "("  ) ); // "(" : Operator Opening Bracket
        operDecodeDic_.insert( make_pair( "::$OP_BC$::" , ")"  ) ); // ")" : Operator Closing Bracket
        operDecodeDic_.insert( make_pair( "::$OP_OO$::" , "["  ) ); // "[" : Operator Opening Orderby Bracket
        operDecodeDic_.insert( make_pair( "::$OP_OC$::" , "]"  ) ); // "]" : Operator Closing Orderby Bracket
        operDecodeDic_.insert( make_pair( "::$OP_NO$::" , "{"  ) ); // "{" : Operator Opening Nearby Bracket
        operDecodeDic_.insert( make_pair( "::$OP_NC$::" , "}"  ) ); // "}" : Operator Closing Nearby Bracket
        operDecodeDic_.insert( make_pair( "::$OP_UP$::" , "^"  ) ); // "^" : Operator Upper Arrow
        operDecodeDic_.insert( make_pair( "::$OP_EX$::" , "\"" ) ); // """ : Operator Exact Bracket
    } // end - initOnlyOnceCore()


    void QueryParser::processEscapeOperator(string& queryString)
    {
        processReplaceAll(queryString, operEncodeDic_);
    } // end - processEscapeOperator()

    void QueryParser::recoverEscapeOperator(string& queryString)
    {
        processReplaceAll(queryString, operDecodeDic_);
    } // end - recoverEscapeOperator()

    void QueryParser::addEscapeCharToOperator(string& queryString)
    {
        string tmpQueryStr;
        for(string::iterator iter = queryString.begin(); iter != queryString.end(); iter++)
        {
            if ( escOperStr_.find(*iter) != string::npos )
                tmpQueryStr += "\\";
            tmpQueryStr += *iter;
        } // end - for
        queryString.swap( tmpQueryStr );
    } // end - addEscapeCharToOperator()

    void QueryParser::removeEscapeChar(string& queryString)
    {
        string tmpQueryStr;
        for(string::iterator iter = queryString.begin(); iter != queryString.end(); iter++)
        {
            if ( *iter == '\\' )
            {
                iter++;
                if ( iter == queryString.end() )
                    break;
                else if ( escOperStr_.find(*iter) != string::npos )
                    tmpQueryStr += *iter;
                else
                {
                    tmpQueryStr += '\\';
                    tmpQueryStr += *iter;
                }
                continue;
            }
            tmpQueryStr += *iter;
        } // end - for
        queryString.swap( tmpQueryStr );

    } // end - addEscapeCharToOperator()

    void QueryParser::normalizeQuery(const string& queryString, string& normString, bool hasUnigramProperty)
    {
        static string openBracket("([{");
        static string closeBracket("]}");
        static string noBoolAfter("(!&|");
        static string noBoolBefore(")&|");
        static string omitChar[2] = { " \"*?", " " };
        string tmpNormString;
        string::const_iterator iter, iterEnd;

        // -----[ Step 1 : Remove initial and trailing spaces and boolean operators. ]
        iter = queryString.begin();
        iterEnd = queryString.end();

        while (iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != string::npos || *iter == '&' || *iter == '|')) ++iter;
        while (iterEnd != iter && (omitChar[hasUnigramProperty].find(*(iterEnd - 1)) != string::npos || *(iterEnd - 1) == '&' || *(iterEnd - 1) == '|' || *(iterEnd - 1) == '!')) --iterEnd;

        // -----[ Step 2 : Remove redundant spaces and do some more tricks ]
        int parenthesesCount = 0;
        while (iter != iterEnd)
        {
            switch (*iter)
            {
                case '!':
                {
                    tmpNormString.push_back(*iter);
                    while (++iter != iterEnd && omitChar[hasUnigramProperty].find(*iter) != string::npos);
                    break;
                } // end - case '!'

                case '&':
                case '|':
                {
                    if (!tmpNormString.empty() && noBoolAfter.find(*tmpNormString.rbegin()) == string::npos)
                        tmpNormString.push_back(*iter);
                    while (++iter != iterEnd && omitChar[hasUnigramProperty].find(*iter) != string::npos);
                    break;
                } // end - case '&' '|'

                case '(':
                {
                    while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != string::npos || closeBracket.find(*iter) != string::npos));
                    if (iter == iterEnd)
                        break;
                    if (*iter == ')')
                    {
                        while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != string::npos || closeBracket.find(*iter) != string::npos));
                        break;
                    }
                    tmpNormString.push_back('(');
                    ++parenthesesCount;
                    break;
                } // end - case '('

                case '[':
                {
                    while (++iter != iterEnd && *iter == ' ');
                    if (iter == iterEnd)
                        break;
                    if (*iter == ']')
                    {
                        while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != string::npos || closeBracket.find(*iter) != string::npos));
                        if (iter != iterEnd && noBoolBefore.find(*iter) == string::npos && !tmpNormString.empty() && noBoolAfter.find(*tmpNormString.rbegin()) == string::npos)
                            tmpNormString.push_back('&');
                        break;
                    }
                    tmpNormString.push_back('[');
                    while (true)
                    {
                        if (*iter != ' ')
                        {
                            tmpNormString.push_back(*iter);
                            while (++iter != iterEnd && *iter != ' ' && *iter != ']')
                                tmpNormString.push_back(*iter);
                            if (iter == iterEnd || *iter == ']')
                                break;
                        }
                        else
                        {
                            while (++iter != iterEnd && *iter == ' ');
                            if (iter == iterEnd || *iter == ']')
                                break;
                            tmpNormString.push_back(' ');
                        }
                    }
                    tmpNormString.push_back(']');
                    if (iter == iterEnd)
                        break;
                    while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != string::npos || closeBracket.find(*iter) != string::npos));
                    if (iter != iterEnd && noBoolBefore.find(*iter) == string::npos)
                        tmpNormString.push_back('&');
                    break;
                } // end - case '['

                case '{':
                {
                    while (++iter != iterEnd && *iter == ' ');
                    if (iter == iterEnd)
                        break;
                    if (*iter == '}')
                    {
                        while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != string::npos || closeBracket.find(*iter) != string::npos));
                        if (iter != iterEnd && noBoolBefore.find(*iter) == string::npos && !tmpNormString.empty() && noBoolAfter.find(*tmpNormString.rbegin()) == string::npos)
                            tmpNormString.push_back('&');
                        break;
                    }
                    tmpNormString.push_back('{');
                    while (true)
                    {
                        if (*iter != ' ')
                        {
                            tmpNormString.push_back(*iter);
                            while (++iter != iterEnd && *iter != ' ' && *iter != '}')
                                tmpNormString.push_back(*iter);
                            if (iter == iterEnd || *iter == '}')
                                break;
                        }
                        else
                        {
                            while (++iter != iterEnd && *iter == ' ');
                            if (iter == iterEnd || *iter == '}')
                                break;
                            tmpNormString.push_back(' ');
                        }
                    }
                    tmpNormString.push_back('}');
                    if (iter == iterEnd)
                        break;
                    while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != string::npos || closeBracket.find(*iter) != string::npos));
                    if (iter == iterEnd)
                        break;
                    if (*iter == '^')
                    {
                        tmpNormString.push_back('^');
                        while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != string::npos || closeBracket.find(*iter) != string::npos));
                        while (iter != iterEnd && isdigit(*iter))
                            tmpNormString.push_back(*iter++);
                        while (iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != string::npos || closeBracket.find(*iter) != string::npos))
                            ++iter;
                    }
                    if (iter != iterEnd && noBoolBefore.find(*iter) == string::npos)
                        tmpNormString.push_back('&');
                    break;
                } // end - case '{'

                case ')':
                {
                    while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != string::npos || closeBracket.find(*iter) != string::npos));
                    if (parenthesesCount)
                    {
                        --parenthesesCount;
                        tmpNormString.push_back(')');
                    }
                    if (iter != iterEnd && noBoolBefore.find(*iter) == string::npos && !tmpNormString.empty() && noBoolAfter.find(*tmpNormString.rbegin()) == string::npos)
                        tmpNormString.push_back('&');
                    break;
                } // end - case ')'

                case '"':
                {
                    if (!hasUnigramProperty)
                    {
                        while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != string::npos || closeBracket.find(*iter) != string::npos));
                        break;
                    }
                    if (++iter == iterEnd)
                        break;
                    if (*iter == '"')
                    {
                        while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != string::npos || closeBracket.find(*iter) != string::npos));
                        if (iter != iterEnd && noBoolBefore.find(*iter) == string::npos && !tmpNormString.empty() && noBoolAfter.find(*tmpNormString.rbegin()) == string::npos)
                            tmpNormString.push_back('&');
                        break;
                    }
                    tmpNormString.push_back('"');
                    do
                    {
                        tmpNormString.push_back(*iter);
                        ++iter;
                    }
                    while (iter != iterEnd && *iter != '"');
                    tmpNormString.push_back('"');
                    if (iter == iterEnd)
                        break;
                    while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != string::npos || closeBracket.find(*iter) != string::npos));
                    if (iter != iterEnd && noBoolBefore.find(*iter) == string::npos)
                        tmpNormString.push_back('&');
                    break;
                } // end - case '"'

                default:
                {
                    tmpNormString.push_back(*iter++);
                    if (iter == iterEnd)
                        break;
                    if (omitChar[hasUnigramProperty].find(*iter) != string::npos)
                    {
                        while (++iter != iterEnd && omitChar[hasUnigramProperty].find(*iter) != string::npos);
                        if (iter != iterEnd && noBoolBefore.find(*iter) == string::npos)
                            tmpNormString.push_back('&');
                        break;
                    }
                    if (openBracket.find(*iter) != string::npos || *iter == '\"' || *iter == '!')
                        tmpNormString.push_back('&');
                    break;
                } // end - default
            } // end - switch()
        } // end - while

        // -----[ Step 3 : Match the unclosed brackets ]
        for (int i = 0; i < parenthesesCount; i++)
            tmpNormString.push_back(')');

        string::iterator it = tmpNormString.end() - 1;
        if (*it == '&')
            tmpNormString.erase(it);

        normString.swap(tmpNormString);

    } // end - normalizeQuery()

    bool QueryParser::parseQuery(
            const izenelib::util::UString& queryUStr,
            QueryTreePtr& queryTree,
            bool unigramFlag,
            bool hasUnigramProperty,
            bool removeChineseSpace
            )
    {
        // Check if given query is restrict word.
        if ( QueryUtility::isRestrictWord( queryUStr ) )
            return false;

        string queryString, normQueryString;
        queryUStr.convertString(queryString, izenelib::util::UString::UTF_8);

        processEscapeOperator(queryString);
        normalizeQuery(queryString, normQueryString, hasUnigramProperty);
        // cout << "query string: ====" << queryString << "====" << endl;
        // cout << "normalized query string: ====" << normQueryString << "====" << endl;

        // Remove redundant space for chinese character.
        if ( removeChineseSpace )
        {
            izenelib::util::UString spaceRefinedQuery;
            spaceRefinedQuery.assign(normQueryString, izenelib::util::UString::UTF_8);
            la::removeRedundantSpaces( spaceRefinedQuery );
            spaceRefinedQuery.convertString(normQueryString, izenelib::util::UString::UTF_8);
        }

        tree_parse_info<> info = ast_parse(normQueryString.c_str(), *this);

        if ( info.match )
        {
            if ( !getQueryTree(info.trees.begin(), queryTree, unigramFlag) )
                return false;
            queryTree->postProcess();
        }

        return info.match;
    } // end - parseQuery()

    bool QueryParser::getAnalyzedQueryTree(
        bool synonymExtension,
        const AnalysisInfo& analysisInfo,
        const izenelib::util::UString& rawUStr,
        QueryTreePtr& analyzedQueryTree,
        string& expandedQueryString,
        bool unigramFlag,
        bool hasUnigramProperty,
        bool isUnigramSearchMode,
        PersonalSearchInfo& personalSearchInfo
    )
    {
        QueryTreePtr tmpQueryTree;
        LAEXInfo laInfo(unigramFlag, synonymExtension, analysisInfo);

        // Apply escaped operator.
        QueryParser::parseQuery( rawUStr, tmpQueryTree, unigramFlag, hasUnigramProperty );
        bool ret = recursiveQueryTreeExtension(tmpQueryTree, laInfo, isUnigramSearchMode, personalSearchInfo, expandedQueryString);
        if ( ret )
        {
            tmpQueryTree->postProcess(isUnigramSearchMode);
            analyzedQueryTree = tmpQueryTree;
        }
        return ret;
    } // end - getPropertyQueryTree()

    bool QueryParser::extendUnigramWildcardTree(QueryTreePtr& queryTree)
    {
        string wildStr( queryTree->keyword_ );
        transform(wildStr.begin(), wildStr.end(), wildStr.begin(), ::tolower);
        UString wildcardUStringQuery(wildStr, UString::UTF_8);
        size_t pos, lastpos = 0;
        bool ret;

        while (lastpos < wildcardUStringQuery.size())
        {
            pos = min(wildcardUStringQuery.find('*', lastpos), wildcardUStringQuery.find('?', lastpos));
            if (pos > lastpos)
            {
                la::TermList termList;
                AnalysisInfo analysisInfo;
                UString tmpUString(wildcardUStringQuery, lastpos, pos - lastpos);

                analysisInfo.analyzerId_ = "la_unigram";
                ret = laManager_->getTermList(
                    tmpUString,
                    analysisInfo,
                    termList );
                if ( !ret ) return false;

                la::TermList::iterator termIter = termList.begin();

                while(termIter != termList.end() ) {
                    QueryTreePtr child(new QueryTree(QueryTree::KEYWORD));
                    child->keywordUString_ = termIter->text_;
                    setKeyword(child, child->keywordUString_);
                    queryTree->insertChild(child);
                    termIter ++;
                }
            }
            if (pos != UString::npos)
            {
                if (wildcardUStringQuery[pos] == '*')
                    queryTree->insertChild(QueryTreePtr(new QueryTree(QueryTree::ASTERISK)));
                else // if ( wildcardUStringQuery[pos] == '?' )
                    queryTree->insertChild(QueryTreePtr(new QueryTree(QueryTree::QUESTION_MARK)));
            }
            lastpos = pos + 1;
        }

        return true;
    } // end - extendWildcardTree

    bool QueryParser::extendTrieWildcardTree(QueryTreePtr& queryTree)
    {
        string wildStr( queryTree->keyword_ );
        transform(wildStr.begin(), wildStr.end(), wildStr.begin(), ::tolower);
        UString wildcardUStringQuery(wildStr, UString::UTF_8);

        vector<UString> wStrList;
        vector<termid_t> wIdList;
        idManager_->getTermListByWildcardPattern(wildcardUStringQuery, wStrList);
        idManager_->getTermIdListByTermStringList(wStrList, wIdList);

        // make child of wildcard
        vector<UString>::iterator strIter = wStrList.begin();
        vector<termid_t>::iterator idIter = wIdList.begin();
        for (; strIter != wStrList.end() && idIter != wIdList.end(); strIter++, idIter++)
        {
            // Skip the restrict term
            if ( QueryUtility::isRestrictId( *idIter ) )
                continue;
            QueryTreePtr child(new QueryTree(QueryTree::KEYWORD));
            child->keywordUString_ = *strIter;
            child->keywordId_ = *idIter;
            strIter->convertString( child->keyword_, UString::UTF_8 );

            queryTree->insertChild( child );
        }
        return true;
    } // end - extendWildcardTree

    bool QueryParser::extendRankKeywords(QueryTreePtr& queryTree, la::TermList& termList)
    {
        QueryTreePtr rankQueryTree;
        //la::TermList::iterator termIter;

        switch ( queryTree->type_ )
        {
            case QueryTree::KEYWORD:
            {
                QueryTreePtr tmpQueryTree( new QueryTree(QueryTree::AND) );
                tmpQueryTree->insertChild(queryTree);
                queryTree = tmpQueryTree;
                // no break;
            }
            case QueryTree::AND:
            case QueryTree::OR:
            {
                for (la::TermList::iterator termIter = termList.begin(); termIter != termList.end(); termIter++)
                {
                    QueryTreePtr child( new QueryTree(QueryTree::RANK_KEYWORD) );
                    if ( !setKeyword(child, termIter->text_) )
                        return false;
                    queryTree->insertChild( child );
                }
                break;
            }
            default:
                return false;
        }
        return true;
    } // end - extendKeyWordTree

    bool QueryParser::extendPersonlSearchTree(QueryTreePtr& queryTree, vector<UString>& userTermList)
    {
        vector<UString>::iterator ustrIter;
        QueryTreePtr extendChild;

        switch ( queryTree->type_ )
        {
            case QueryTree::KEYWORD:
            {
                QueryTreePtr tmpQueryTree( new QueryTree(QueryTree::AND) );
                tmpQueryTree->insertChild(queryTree);
                queryTree = tmpQueryTree;
                // no break;
            }
            case QueryTree::AND:
            case QueryTree::OR:
            {
                if (queryTree->type_ == QueryTree::AND)
                {
                    extendChild.reset(new QueryTree(QueryTree::AND_PERSONAL));
                }
                else // QueryTree::OR
                {
                    extendChild.reset(new QueryTree(QueryTree::OR_PERSONAL));
                }

                ustrIter = userTermList.begin();
                for ( ; ustrIter != userTermList.end(); ustrIter++)
                {
                    QueryTreePtr subChild( new QueryTree(QueryTree::KEYWORD) );
                    if ( !setKeyword(subChild, *ustrIter) ) {
                        string str;
                        (*ustrIter).convertString(str, UString::UTF_8);
                        cout << "[QueryParser::extendPersonlSearchTree] Failed to set keyword: " << str << endl;
                        continue; // todo, apply LA
                    }
                    extendChild->insertChild( subChild );
                }

                queryTree->insertChild(extendChild);

                break;
            }
            default:
            {
                cout << "[QueryParser::extendPersonlSearchTree] Error type of query tree." << endl;
                return false;
            }
        }

        return true;
    } // end - extendPersonlSearchTree

    bool QueryParser::recursiveQueryTreeExtension(QueryTreePtr& queryTree, const LAEXInfo& laInfo, bool isUnigramSearchMode,
            PersonalSearchInfo& personalSearchInfo, string& expandedQueryString)
    {
        switch (queryTree->type_)
        {
        case QueryTree::KEYWORD:
        {
            QueryTreePtr tmpQueryTree;
            UString keywordString = queryTree->keywordUString_;
            UString analyzedUStr;
            AnalysisInfo analysisInfo;
            if (!isUnigramSearchMode || laInfo.analysisInfo_.analyzerId_.empty())
            {
                // the addtional case that la is empty: e.g. DATE property
                analysisInfo = laInfo.analysisInfo_;
                if (analysisInfo.analyzerId_ == "la_sia_with_unigram")
                {
                    analysisInfo.analyzerId_ = "la_sia";
                }
            }
            else
            {
                // search is based on unigram terms
                analysisInfo.analyzerId_ = "la_unigram";
            }

            if ( !laManager_->getExpandedQuery(
                queryTree->keywordUString_,
                analysisInfo,
                true,
                laInfo.synonymExtension_,
                analyzedUStr))
            {
                cout<<"Error LA not found: "<<analysisInfo.toString()<<endl;
            }
            string escAddedStr;
            analyzedUStr.convertString(escAddedStr, UString::UTF_8);
            expandedQueryString += escAddedStr; // for search cache identity
            analyzedUStr.assign(escAddedStr, UString::UTF_8);
            if (!QueryParser::parseQuery(analyzedUStr, tmpQueryTree, laInfo.unigramFlag_, false))
                return false;
            queryTree = tmpQueryTree;

            // Extend query tree with word segment terms for ranking, while search will perform on unigram terms
            if ( isUnigramSearchMode )
            {
                cout<<"* Extend rank keywords."<<endl;
                analysisInfo = laInfo.analysisInfo_;
                if (!laInfo.analysisInfo_.analyzerId_.empty())
                    analysisInfo.analyzerId_ = "la_sia"; //xxx

                la::TermList termList;
                if ( !laManager_->getTermList(keywordString, analysisInfo, termList) )
                {
                    cout<<"Error LA not found: "<<analysisInfo.toString()<<endl;
                }

                if (!extendRankKeywords(queryTree, termList))
                {
                    cout<<"** failed to extend rank keywords."<<endl;
                    return false;
                }
            }

            // Extend query tree for personalized search
            if (personalSearchInfo.enabled)
            {
                cout << "[QueryParser] Extend query tree for personalized search " << endl;

                User& user = personalSearchInfo.user;
                for (User::PropValueMap::iterator iter = user.propValueMap_.begin();
                        iter != user.propValueMap_.end(); iter ++)
                {
                    UString itemValue = iter->second; // apply LA ? xxx

                    vector<UString> userTermList;
                    userTermList.push_back(itemValue);

                    if (!extendPersonlSearchTree(queryTree, userTermList)) {
                        //return false;
                    }
                }
            }

            break;
        }
        // Skip Language Analyzing
        case QueryTree::UNIGRAM_WILDCARD:
        case QueryTree::TRIE_WILDCARD:
        // Unlike NEARBY and ORDERED query, there is no need to analyze EXACT query.
        // EXACT query is processed via unigram.
        case QueryTree::EXACT:
            break;
        case QueryTree::NEARBY:
        case QueryTree::ORDER:
            return tokenizeBracketQuery(
                    queryTree->keyword_,
                    laInfo.analysisInfo_,
                    queryTree->type_,
                    queryTree,
                    queryTree->distance_);
        default:
            for (QTIter iter = queryTree->children_.begin();
                    iter != queryTree->children_.end(); iter++)
                recursiveQueryTreeExtension(*iter, laInfo, isUnigramSearchMode, personalSearchInfo, expandedQueryString);
        } // end - switch(queryTree->type_)
        return true;
    } // end - recursiveQueryTreeExtension()

    bool QueryParser::getQueryTree(iter_t const& i, QueryTreePtr& queryTree, bool unigramFlag)
    {
        if ( i->value.id() == stringQueryID )
            return processKeywordAssignQuery(i, queryTree, unigramFlag);
        else if  (i->value.id() == exactQueryID )
            return processExactQuery(i, queryTree);
        else if ( i->value.id() == orderedQueryID )
            return processBracketQuery(i, QueryTree::ORDER, queryTree, unigramFlag);
        else if ( i->value.id() == nearbyQueryID )
            return processBracketQuery(i, QueryTree::NEARBY, queryTree, unigramFlag);
        else if ( i->value.id() == boolQueryID )
            return processBoolQuery(i, queryTree, unigramFlag);
        else
            return processChildTree(i, queryTree, unigramFlag);
    } // end - getQueryTree()

    bool QueryParser::processKeywordAssignQuery( iter_t const& i, QueryTreePtr& queryTree, bool unigramFlag)
    {
        bool ret = true;

        string tmpString( i->value.begin(), i->value.end() );
        recoverEscapeOperator( tmpString );
        QueryTreePtr tmpQueryTree(new QueryTree(QueryTree::KEYWORD));
        if ( !setKeyword(tmpQueryTree, tmpString) )
            return false;

        if ( tmpString.find('*') != string::npos || tmpString.find('?') != string::npos )
        {
            if (unigramFlag)
            {
                tmpQueryTree->type_ = QueryTree::UNIGRAM_WILDCARD;
                ret = extendUnigramWildcardTree(tmpQueryTree);
            }
            else
            {
                tmpQueryTree->type_ = QueryTree::TRIE_WILDCARD;
                ret = extendTrieWildcardTree(tmpQueryTree);
            }
        }

        queryTree = tmpQueryTree;
        return ret;
    } // end - processKeywordAssignQuery()

    bool QueryParser::processExactQuery(iter_t const& i, QueryTreePtr& queryTree)
    {
        bool ret;
        QueryTreePtr tmpQueryTree(new QueryTree(QueryTree::EXACT));

        // Store String value of first child into keyword_
        iter_t childIter = i->children.begin();
        string tmpString( childIter->value.begin(), childIter->value.end() );
        setKeyword(tmpQueryTree, tmpString);

        // Make child query tree with default-tokenized terms.
        la::TermList termList;
        AnalysisInfo analysisInfo;

        // Unigram analyzing is used for any kinds of query for "EXACT" query.
        analysisInfo.analyzerId_ = "la_unigram";
        ret = laManager_->getTermList(
            tmpQueryTree->keywordUString_,
            analysisInfo,
//            false,
            termList );
        if ( !ret ) return false;

        cout << "ExactQuery Term : " << la::to_utf8(tmpQueryTree->keywordUString_) << endl;
        cout << "ExactQuery TermList size : " << termList.size() << endl;
        cout << "------------------------------" << endl << termList << endl;

        la::TermList::iterator termIter = termList.begin();

        while(termIter != termList.end() ) {
            QueryTreePtr child(new QueryTree(QueryTree::KEYWORD));
            child->keywordUString_ = termIter->text_;
            setKeyword(child, child->keywordUString_);
            tmpQueryTree->insertChild(child);
            termIter ++;
        }

        queryTree = tmpQueryTree;
        return true;
    }

    bool QueryParser::processBracketQuery(iter_t const& i, QueryTree::QueryType queryType, QueryTreePtr& queryTree, bool unigramFlag)
    {
        // Store String value of first child into keyword_
        iter_t childIter = i->children.begin();
        string queryStr( childIter->value.begin(), childIter->value.end() );

        // Use default tokenizer
        AnalysisInfo analysisInfo;
        int distance = 20;
        if ( queryType == QueryTree::NEARBY )
        {
            // Store distance
            iter_t distIter = i->children.begin()+1;
            if ( distIter != i->children.end() )
            {
                string distStr( distIter->value.begin(), distIter->value.end() );
                distance = atoi( distStr.c_str() );
            }
            else
                distance = 20;
        }
        return tokenizeBracketQuery(queryStr, analysisInfo, queryType, queryTree, distance);
    } // end - processBracketQuery()

    bool QueryParser::tokenizeBracketQuery(
            const string& queryStr,
            const AnalysisInfo& analysisInfo,
            QueryTree::QueryType queryType,
            QueryTreePtr& queryTree,
            int distance)
    {
        bool ret;
        QueryTreePtr tmpQueryTree(new QueryTree(queryType));
        setKeyword(tmpQueryTree, queryStr);

        // Make child query tree with default-tokenized terms.
        la::TermList termList;
        la::TermList::iterator termIter;
        ret = laManager_->getTermList(
            tmpQueryTree->keywordUString_,
            analysisInfo,
//            false,
            termList );
        if ( !ret ) return false;

        for ( termIter = termList.begin(); termIter != termList.end(); termIter++ )
        {
            // Find restrict Word
            if ( QueryUtility::isRestrictWord( termIter->text_ ) )
                continue;

            QueryTreePtr child(new QueryTree(QueryTree::KEYWORD));
            termIter->text_.convertString(child->keyword_, UString::UTF_8);
            removeEscapeChar(child->keyword_);
            setKeyword(child, child->keyword_);
            tmpQueryTree->insertChild( child );
        }

        if ( queryType == QueryTree::NEARBY )
            tmpQueryTree->distance_ = distance;

        queryTree = tmpQueryTree;
        return true;
    } // end - tokenizeBracketQuery()

    bool QueryParser::processBoolQuery(iter_t const& i, QueryTreePtr& queryTree, bool unigramFlag)
    {
        QueryTree::QueryType queryType;
        switch ( *(i->value.begin() ) )
        {
        case '&':
            queryType = QueryTree::AND;
            break;
        case '|':
            queryType = QueryTree::OR;
            break;
        default:
            queryType = QueryTree::UNKNOWN;
            return false;
        } // end - switch
        QueryTreePtr tmpQueryTree( new QueryTree(queryType) );

        // Retrieve child query tree and insert it to the children_ list of tmpQueryTree.
        iter_t iter = i->children.begin();
        for (; iter != i->children.end(); iter++)
        {
            QueryTreePtr tmpChildQueryTree;
            if ( getQueryTree( iter, tmpChildQueryTree, unigramFlag) )
                tmpQueryTree->insertChild( tmpChildQueryTree );
            else
                return false;
        }

        // Merge unnecessarily divided bool-trees
        //   - Children are always two : Binary Tree
        QTIter first = tmpQueryTree->children_.begin();
        QTIter second = first;
        second++;
        if ( (*first)->type_ == queryType )
        {
            // use first child tree as a root tree.
            if ( (*second)->type_ == queryType )
                // Move all children in second node into first ( Insert : first->end )
                (*first)->children_.splice( (*first)->children_.end(), (*second)->children_ );
            else
                // Move second node into children in first ( Insert : first->end )
                (*first)->children_.push_back( *second );

            tmpQueryTree = *first;
        }
        else if ((*second)->type_ == queryType )
        {
            // Move first node into children in second. ( Insert : second->begin )
            (*second)->children_.push_front( *first );
            tmpQueryTree = *second;
        }

        queryTree = tmpQueryTree;
        return true;
    } // end - processBoolQuery()

    bool QueryParser::processChildTree(iter_t const& i, QueryTreePtr& queryTree, bool unigramFlag)
    {
        QueryTree::QueryType queryType;
        if ( i->value.id() == notQueryID ) queryType = QueryTree::NOT;
        else queryType = QueryTree::UNKNOWN;

        QueryTreePtr tmpQueryTree( new QueryTree(queryType) );

        // Retrieve child query tree and insert it to the children_ list of tmpQueryTree.
        iter_t iter = i->children.begin();
        for (; iter != i->children.end(); iter++)
        {
            QueryTreePtr tmpChildQueryTree;
            if ( getQueryTree( iter, tmpChildQueryTree, unigramFlag) )
                tmpQueryTree->insertChild( tmpChildQueryTree );
            else
                return false;
        }
        queryTree = tmpQueryTree;
        return true;
    } // end - processChildTree

    bool QueryParser::processReplaceAll(string& queryString, boost::unordered_map<string,string>& dic)
    {
        boost::unordered_map<string,string>::const_iterator iter = dic.begin();
        for(; iter != dic.end(); iter++)
            boost::replace_all(queryString, iter->first, iter->second);
        return true;
    } // end - process Rreplace All process

    bool QueryParser::setKeyword(QueryTreePtr& queryTree, const string& utf8Str)
    {
        if ( !queryTree )
            return false;
        queryTree->keyword_ = utf8Str;
        queryTree->keywordUString_.assign( utf8Str , UString::UTF_8 );
        idManager_->getTermIdByTermString( queryTree->keywordUString_ , queryTree->keywordId_ );
        return !QueryUtility::isRestrictWord( queryTree->keywordUString_ );
    } // end - setKeyword()

    bool QueryParser::setKeyword(QueryTreePtr& queryTree, const izenelib::util::UString& uStr)
    {
        if ( !queryTree )
            return false;
        queryTree->keywordUString_.assign( uStr );
        queryTree->keywordUString_.convertString(queryTree->keyword_ , UString::UTF_8);
        idManager_->getTermIdByTermString( queryTree->keywordUString_ , queryTree->keywordId_ );
        return !QueryUtility::isRestrictWord( queryTree->keywordUString_ );
    } // end - setKeyword()

} // end - namespace sf1r
