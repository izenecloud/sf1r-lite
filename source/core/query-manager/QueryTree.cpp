///
/// @brief  source file of Query Tree Structure
/// @author Dohyun Yun
/// @date   2010.06.17 (First Created)
///

#include "QueryTree.h"

#include <sstream>

namespace sf1r {

    QueryTree::QueryTree() : type_(QueryTree::UNKNOWN), keywordId_(0)
    {
    } // end - QueryTree()

    QueryTree::QueryTree(QueryType type) : type_(type), keywordId_(0)
    {
    } // end - QueryTree()

    void QueryTree::insertChild(const boost::shared_ptr<QueryTree>& childQueryTree)
    {
        children_.push_back( childQueryTree );
    } // end - insertChild()

    void QueryTree::postProcess()
    {
        unsigned int pos = 0;
        queryTermIdSet_.clear();
        queryTermIdList_.clear();
        propertyTermInfo_.clear();
        recursivePreProcess(queryTermIdSet_, queryTermIdList_, propertyTermInfo_, pos);
    } // end - postProcess()

    void QueryTree::getQueryTermIdSet(std::set<termid_t>& queryTermIdSet) const
    {
        queryTermIdSet = queryTermIdSet_;
    } // end - getQueryTermIdSet()

    void QueryTree::getPropertyTermInfo(PropertyTermInfo& propertyTermInfo) const
    {
        propertyTermInfo = propertyTermInfo_;
    } // end - getPropertyTermInfo()

    void QueryTree::getLeafTermIdList(std::vector<termid_t>& leafTermIdList) const
    {
        leafTermIdList = queryTermIdList_;
    } // end - getLeafTermIdList()


    void QueryTree::print(std::ostream& out, int tabLevel) const
    {
        using namespace std;
        stringstream ss;

        for(int i = 0; i < tabLevel; i++) ss << " ";
        ss << "[";
        switch( type_ )
        {
            case QueryTree::KEYWORD:
                ss << "KEYWORD  : (" << keyword_ << "," << keywordId_ << ") ]" << endl;
                break;
            case QueryTree::UNIGRAM_WILDCARD:
                ss << "UNIGRAM WILDCARD :" << keyword_ << "]" << endl;
                break;
            case QueryTree::TRIE_WILDCARD:
                ss << "TRIE WILDCARD :" << keyword_ << "]" << endl;
                break;
            case QueryTree::AND:
                ss << "AND      : ]" << endl;
                break;
            case QueryTree::OR:
                ss << "OR       : ]" << endl;
                break;
            case QueryTree::PERSONAL_AND:
                ss << "PERSONAL_AND      : ]" << endl;
                break;
            case QueryTree::PERSONAL_OR:
                ss << "PERSONAL_OR       : ]" << endl;
                break;
            case QueryTree::NOT:
                ss << "NOT      : ]" << endl;
                break;
            case QueryTree::EXACT:
                ss << "EXACT    :" << keyword_ << "]" << endl;
                break;
            case QueryTree::ORDER:
                ss << "ORDER    :" << keyword_ << "]" << endl;
                break;
            case QueryTree::NEARBY:
                ss << "NEARBY   :" << keyword_ << ":distance(" << distance_ << ") ]" << endl;
                break;
            case QueryTree::ASTERISK:
                ss << "ASTERISK       : ]" << endl;
                break;
            case QueryTree::QUESTION_MARK:
                ss << "QUESTION_MARK       : ]" << endl;
                break;
            default:
                ss << "UNKNOWN  : ]" << endl;
                break;
        }
        out << ss.str();

        QTConstIter iter = children_.begin();
        for(; iter != children_.end(); iter++)
            (*iter)->print(out, tabLevel + 3 );
    } // print()

    void QueryTree::recursivePreProcess(
            std::set<termid_t>& queryTermIdSet,
            std::vector<termid_t>& queryTermIdList,
            PropertyTermInfo& propertyTermInfo,
            unsigned int& pos)
    {
        if ( type_ == QueryTree::KEYWORD )
        {
            queryTermIdSet.insert( keywordId_ );
            queryTermIdList.push_back( keywordId_ );
            propertyTermInfo.dealWithTerm( keywordUString_ , keywordId_ , pos++);
        } // if
        else if (children_.size() > 0 )
        {
            for(QTIter childIter = children_.begin();
                    childIter != children_.end(); childIter++)
                (*childIter)->recursivePreProcess( queryTermIdSet , queryTermIdList, propertyTermInfo , pos );
        } // end - else if
    } // end - recursivePreProcess()

} // end - namespace sf1r

