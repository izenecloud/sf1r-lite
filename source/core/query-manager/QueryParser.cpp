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

#define USTRING(str) UString(str, UString::UTF_8)

namespace sf1r
{
    using namespace std;
    using izenelib::util::UString;

    // Declare static variables
    UString QueryParser::operUStr_;
    UString QueryParser::escOperUStr_;
    vector<pair<UString , UString> > QueryParser::operEncodeDic_;
    vector<pair<UString , UString> > QueryParser::operDecodeDic_;

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
        operUStr_.assign("&|!(){}[]^\"", UString::UTF_8);
        escOperUStr_ = operUStr_; escOperUStr_.push_back('\\');

        operEncodeDic_.push_back( make_pair( USTRING("\\\\") , USTRING("::$OP_SL$::") ) ); // "\\" : Operator Slash
        operEncodeDic_.push_back( make_pair( USTRING("\\&")  , USTRING("::$OP_AN$::") ) ); // "\ " : Operator AND
        operEncodeDic_.push_back( make_pair( USTRING("\\|")  , USTRING("::$OP_OR$::") ) ); // "\|" : Operator OR
        operEncodeDic_.push_back( make_pair( USTRING("\\!")  , USTRING("::$OP_NT$::") ) ); // "\!" : Operator NOT
        operEncodeDic_.push_back( make_pair( USTRING("\\(")  , USTRING("::$OP_BO$::") ) ); // "\(" : Operator Opening Bracket
        operEncodeDic_.push_back( make_pair( USTRING("\\)")  , USTRING("::$OP_BC$::") ) ); // "\)" : Operator Closing Bracket
        operEncodeDic_.push_back( make_pair( USTRING("\\[")  , USTRING("::$OP_OO$::") ) ); // "\[" : Operator Opening Orderby Bracket
        operEncodeDic_.push_back( make_pair( USTRING("\\]")  , USTRING("::$OP_OC$::") ) ); // "\]" : Operator Closing Orderby Bracket
        operEncodeDic_.push_back( make_pair( USTRING("\\{")  , USTRING("::$OP_NO$::") ) ); // "\{" : Operator Opening Nearby Bracket
        operEncodeDic_.push_back( make_pair( USTRING("\\}")  , USTRING("::$OP_NC$::") ) ); // "\}" : Operator Closing Nearby Bracket
        operEncodeDic_.push_back( make_pair( USTRING("\\^")  , USTRING("::$OP_UP$::") ) ); // "\^" : Operator Upper Arrow
        operEncodeDic_.push_back( make_pair( USTRING("\\\"") , USTRING("::$OP_EX$::") ) ); // "\"" : Operator Exact Bracket

        operDecodeDic_.push_back( make_pair( USTRING("::$OP_SL$::") , USTRING("\\") ) ); // "\" : Operator Slash
        operDecodeDic_.push_back( make_pair( USTRING("::$OP_AN$::") , USTRING("&")  ) ); // " " : Operator AND
        operDecodeDic_.push_back( make_pair( USTRING("::$OP_OR$::") , USTRING("|")  ) ); // "|" : Operator OR
        operDecodeDic_.push_back( make_pair( USTRING("::$OP_NT$::") , USTRING("!")  ) ); // "!" : Operator NOT
        operDecodeDic_.push_back( make_pair( USTRING("::$OP_BO$::") , USTRING("(")  ) ); // "(" : Operator Opening Bracket
        operDecodeDic_.push_back( make_pair( USTRING("::$OP_BC$::") , USTRING(")")  ) ); // ")" : Operator Closing Bracket
        operDecodeDic_.push_back( make_pair( USTRING("::$OP_OO$::") , USTRING("[")  ) ); // "[" : Operator Opening Orderby Bracket
        operDecodeDic_.push_back( make_pair( USTRING("::$OP_OC$::") , USTRING("]")  ) ); // "]" : Operator Closing Orderby Bracket
        operDecodeDic_.push_back( make_pair( USTRING("::$OP_NO$::") , USTRING("{")  ) ); // "{" : Operator Opening Nearby Bracket
        operDecodeDic_.push_back( make_pair( USTRING("::$OP_NC$::") , USTRING("}")  ) ); // "}" : Operator Closing Nearby Bracket
        operDecodeDic_.push_back( make_pair( USTRING("::$OP_UP$::") , USTRING("^")  ) ); // "^" : Operator Upper Arrow
        operDecodeDic_.push_back( make_pair( USTRING("::$OP_EX$::") , USTRING("\"") ) ); // """ : Operator Exact Bracket
    } // end - initOnlyOnceCore()


    void QueryParser::processEscapeOperator(UString& queryUstr)
    {
        processReplaceAll(queryUstr, operEncodeDic_);
    } // end - processEscapeOperator()

    void QueryParser::recoverEscapeOperator(UString& queryUstr)
    {
        processReplaceAll(queryUstr, operDecodeDic_);
    } // end - recoverEscapeOperator()

    void QueryParser::addEscapeCharToOperator(UString& queryUstr)
    {
        UString tmpQueryUStr;
        for(UString::iterator iter = queryUstr.begin(); iter != queryUstr.end(); iter++)
        {
            if ( escOperUStr_.find(*iter) != UString::npos )
                tmpQueryUStr.push_back('\\');
            tmpQueryUStr.push_back(*iter);
        } // end - for
        queryUstr.swap( tmpQueryUStr );
    } // end - addEscapeCharToOperator()

    void QueryParser::removeEscapeChar(UString& queryUstr)
    {
        UString tmpQueryUStr;
        for(UString::iterator iter = queryUstr.begin(); iter != queryUstr.end(); iter++)
        {
            if ( *iter == '\\' )
            {
                iter++;
                if ( iter == queryUstr.end() )
                    break;
                else if ( escOperUStr_.find(*iter) != UString::npos )
                    tmpQueryUStr.push_back(*iter);
                else
                {
                    tmpQueryUStr.push_back('\\');
                    tmpQueryUStr.push_back(*iter);
                }
                continue;
            }
            tmpQueryUStr.push_back(*iter);
        } // end - for
        queryUstr.swap( tmpQueryUStr );

    } // end - addEscapeCharToOperator()

    void QueryParser::normalizeQuery(const UString& queryUstr, UString& normUStr, bool hasUnigramProperty)
    {
        static const UString openBracket("([{", UString::UTF_8);
        static const UString closeBracket("]}^", UString::UTF_8);
        static const UString notBeforeBool("(!&|", UString::UTF_8);
        static const UString notAfterBool(")&|", UString::UTF_8);
        static const UString omitChar[2] = {USTRING( " \"*?" ), USTRING( " " )};
        UString tmpNormUStr;
        UString::const_iterator iter, iterEnd;

        // -----[ Step 1 : Remove initial and trailing spaces and boolean operators. ]
        iter = queryUstr.begin();
        iterEnd = queryUstr.end();

        while (iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || *iter == '&' || *iter == '|')) ++iter;
        while (iterEnd != iter && (omitChar[hasUnigramProperty].find(*(iterEnd - 1)) != UString::npos || *(iterEnd - 1) == '&' || *(iterEnd - 1) == '|' || *(iterEnd - 1) == '!')) --iterEnd;

        // -----[ Step 2 : Remove redundant spaces and do some more tricks ]
        int parenthesesCount = 0;
        while (iter != iterEnd)
        {
            switch (*iter)
            {
                case '!':
                {
                    tmpNormUStr.push_back(*iter);
                    while (++iter != iterEnd && omitChar[hasUnigramProperty].find(*iter) != UString::npos);
                    break;
                } // end - case '!'

                case '&':
                case '|':
                {
                    if (!tmpNormUStr.empty() && notBeforeBool.find(*tmpNormUStr.rbegin()) == UString::npos)
                        tmpNormUStr.push_back(*iter);
                    while (++iter != iterEnd && omitChar[hasUnigramProperty].find(*iter) != UString::npos);
                    break;
                } // end - case '&' '|'

                case '(':
                {
                    while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos));
                    if (iter == iterEnd)
                        break;
                    if (*iter == ')')
                    {
                        while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos));
                        break;
                    }
                    tmpNormUStr.push_back('(');
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
                        while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos));
                        if (iter != iterEnd && notAfterBool.find(*iter) == UString::npos && !tmpNormUStr.empty() && notBeforeBool.find(*tmpNormUStr.rbegin()) == UString::npos)
                            tmpNormUStr.push_back('&');
                        break;
                    }
                    tmpNormUStr.push_back('[');
                    while (true)
                    {
                        if (*iter != ' ')
                        {
                            tmpNormUStr.push_back(*iter);
                            while (++iter != iterEnd && *iter != ' ' && *iter != ']')
                                tmpNormUStr.push_back(*iter);
                            if (iter == iterEnd || *iter == ']')
                                break;
                        }
                        else
                        {
                            while (++iter != iterEnd && *iter == ' ');
                            if (iter == iterEnd || *iter == ']')
                                break;
                            tmpNormUStr.push_back(' ');
                        }
                    }
                    tmpNormUStr.push_back(']');
                    if (iter == iterEnd)
                        break;
                    while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos));
                    if (iter != iterEnd && notAfterBool.find(*iter) == UString::npos)
                        tmpNormUStr.push_back('&');
                    break;
                } // end - case '['

                case '{':
                {
                    while (++iter != iterEnd && *iter == ' ');
                    if (iter == iterEnd)
                        break;
                    if (*iter == '}')
                    {
                        while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos));
                        if (iter != iterEnd && notAfterBool.find(*iter) == UString::npos && !tmpNormUStr.empty() && notBeforeBool.find(*tmpNormUStr.rbegin()) == UString::npos)
                            tmpNormUStr.push_back('&');
                        break;
                    }
                    tmpNormUStr.push_back('{');
                    while (true)
                    {
                        if (*iter != ' ')
                        {
                            tmpNormUStr.push_back(*iter);
                            while (++iter != iterEnd && *iter != ' ' && *iter != '}')
                                tmpNormUStr.push_back(*iter);
                            if (iter == iterEnd || *iter == '}')
                                break;
                        }
                        else
                        {
                            while (++iter != iterEnd && *iter == ' ');
                            if (iter == iterEnd || *iter == '}')
                                break;
                            tmpNormUStr += ' ';
                        }
                    }
                    tmpNormUStr.push_back('}');
                    if (iter == iterEnd)
                        break;
                    while (++iter != iterEnd && *iter != '^' && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos));
                    if (iter == iterEnd)
                        break;
                    if (*iter == '^')
                    {
                        tmpNormUStr.push_back('^');
                        while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos));
                        while (iter != iterEnd && isdigit(*iter))
                            tmpNormUStr.push_back(*iter++);
                        while (iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos))
                            ++iter;
                    }
                    if (iter != iterEnd && notAfterBool.find(*iter) == UString::npos)
                        tmpNormUStr.push_back('&');
                    break;
                } // end - case '{'

                case ')':
                {
                    while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos));
                    if (parenthesesCount)
                    {
                        --parenthesesCount;
                        UString::iterator it = tmpNormUStr.end() - 1;
                        while (!tmpNormUStr.empty() && (*it == '&' || *it == '!' || *it == '|'))
                            tmpNormUStr.erase(it--);
                        if (tmpNormUStr.empty())
                            break;
                        if (*it == '(')
                            tmpNormUStr.erase(it);
                        else
                            tmpNormUStr.push_back(')');
                    }
                    if (iter != iterEnd && notAfterBool.find(*iter) == UString::npos && !tmpNormUStr.empty() && notBeforeBool.find(*tmpNormUStr.rbegin()) == UString::npos)
                        tmpNormUStr.push_back('&');
                    break;
                } // end - case ')'

                case '"':
                {
                    if (!hasUnigramProperty)
                    {
                        while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos));
                        break;
                    }
                    if (++iter == iterEnd)
                        break;
                    if (*iter == '"')
                    {
                        while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos));
                        if (iter != iterEnd && notAfterBool.find(*iter) == UString::npos && !tmpNormUStr.empty() && notBeforeBool.find(*tmpNormUStr.rbegin()) == UString::npos)
                            tmpNormUStr.push_back('&');
                        break;
                    }
                    tmpNormUStr.push_back('"');
                    do
                    {
                        tmpNormUStr.push_back(*iter);
                        ++iter;
                    }
                    while (iter != iterEnd && *iter != '"');
                    tmpNormUStr.push_back('"');
                    if (iter == iterEnd)
                        break;
                    while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos));
                    if (iter != iterEnd && notAfterBool.find(*iter) == UString::npos)
                        tmpNormUStr.push_back('&');
                    break;
                } // end - case '"'

                default:
                {
                    tmpNormUStr.push_back(*iter++);
                    if (iter == iterEnd)
                        break;
                    if (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos)
                    {
                        while (++iter != iterEnd && (omitChar[hasUnigramProperty].find(*iter) != UString::npos || closeBracket.find(*iter) != UString::npos));
                        if (iter != iterEnd && notAfterBool.find(*iter) == UString::npos)
                            tmpNormUStr.push_back('&');
                        break;
                    }
                    if (openBracket.find(*iter) != UString::npos || *iter == '\"' || *iter == '!')
                        tmpNormUStr.push_back('&');
                    break;
                } // end - default
            } // end - switch()
        } // end - while

        // -----[ Step 3 : Match the unclosed brackets ]
        UString::iterator it = tmpNormUStr.end() - 1;
        while (!tmpNormUStr.empty() && (*it == '&' || *it == '!' || *it == '|'))
            tmpNormUStr.erase(it--);
        for (int i = 0; i < parenthesesCount; i++)
            tmpNormUStr.push_back(')');

        normUStr.swap(tmpNormUStr);

    } // end - normalizeQuery()

    bool QueryParser::parseQuery(
            const UString& queryUStr,
            QueryTreePtr& queryTree,
            bool unigramFlag,
            bool hasUnigramProperty,
            bool removeChineseSpace
            )
    {
        // Check if given query is restrict word.
        if ( QueryUtility::isRestrictWord( queryUStr ) )
            return false;

        UString tmpQueryUStr = queryUStr, normQueryUStr;
        processEscapeOperator(tmpQueryUStr);
        normalizeQuery(tmpQueryUStr, normQueryUStr, hasUnigramProperty);

        // Remove redundant space for chinese character.
        if ( removeChineseSpace )
        {
            la::removeRedundantSpaces(normQueryUStr);
        }

        tree_parse_info<const uint16_t *> info = ast_parse(normQueryUStr.c_str(), *this);

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
        const UString& rawUStr,
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
        UString wildcardUStringQuery(queryTree->keywordUString_);
        Algorithm<UString>::to_lower(wildcardUStringQuery);
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
        UString wildcardUStringQuery(queryTree->keywordUString_);
        Algorithm<UString>::to_lower(wildcardUStringQuery);

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
                    queryTree->keywordUString_,
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

        UString tmpUStr( i->value.begin(), i->value.end() );
        recoverEscapeOperator( tmpUStr );
        QueryTreePtr tmpQueryTree(new QueryTree(QueryTree::KEYWORD));
        if ( !setKeyword(tmpQueryTree, tmpUStr) )
            return false;

        if ( tmpUStr.find('*') != UString::npos || tmpUStr.find('?') != UString::npos )
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
        UString tmpUStr( childIter->value.begin(), childIter->value.end() );
        setKeyword(tmpQueryTree, tmpUStr);

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
        UString queryUStr( childIter->value.begin(), childIter->value.end() );

        // Use default tokenizer
        AnalysisInfo analysisInfo;
        int distance = 20;
        if ( queryType == QueryTree::NEARBY )
        {
            // Store distance
            iter_t distIter = i->children.begin()+1;
            if ( distIter != i->children.end() )
            {
                UString distUStr( distIter->value.begin(), distIter->value.end() );
                string distStr;
                distUStr.convertString(distStr, UString::UTF_8);
                distance = atoi( distStr.c_str() );
            }
            else
                distance = 20;
        }
        return tokenizeBracketQuery(queryUStr, analysisInfo, queryType, queryTree, distance);
    } // end - processBracketQuery()

    bool QueryParser::tokenizeBracketQuery(
            const UString& queryUStr,
            const AnalysisInfo& analysisInfo,
            QueryTree::QueryType queryType,
            QueryTreePtr& queryTree,
            int distance)
    {
        bool ret;
        QueryTreePtr tmpQueryTree(new QueryTree(queryType));
        setKeyword(tmpQueryTree, queryUStr);

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
            child->keywordUString_ = termIter->text_;
            removeEscapeChar(child->keywordUString_);
            setKeyword(child, child->keywordUString_);
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

    bool QueryParser::processReplaceAll(UString& queryUstr, vector<pair<UString,UString> >& dic)
    {
        vector<pair<UString,UString> >::const_iterator iter = dic.begin();
        for(; iter != dic.end(); iter++)
            boost::replace_all(queryUstr, iter->first, iter->second);
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

    bool QueryParser::setKeyword(QueryTreePtr& queryTree, const UString& uStr)
    {
        if ( !queryTree )
            return false;
        queryTree->keywordUString_.assign( uStr );
        queryTree->keywordUString_.convertString(queryTree->keyword_ , UString::UTF_8);
        idManager_->getTermIdByTermString( queryTree->keywordUString_ , queryTree->keywordId_ );
        return !QueryUtility::isRestrictWord( queryTree->keywordUString_ );
    } // end - setKeyword()

} // end - namespace sf1r
