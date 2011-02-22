///
/// @file ActionItem.h
/// @brief header file of ActionItem classes
/// @author Dohyun Yun
/// @date 2008-06-05
/// @details
/// - Log
///     - 2009.06.11 Add getTaxonomyItem() & setTaxonomyItem() in commonInformation class by Dohyun Yun
///     - 2009.06.18 Add getTaxonomyIdList() & setTaxonomyIdList() in search keyword action item class by Dohyun Yun.
///                  From now on, SearchKeywordActionItem will be used for keyword search and taxonomy process.
///     - 2009.08.10 Change all the actionitems and remove the hierarchical structures. by Dohyun Yun
///     - 2009.08.21 Remove MF related codes by Dohyun Yun.
///     - 2009.09.01 Add operator =  in KeywordSearchActionItem to get the values from LabelClickActionItem
///     - 2009.09.08 Add serialize() function to KeywordSearchActionItem for using it to the MLRUCacue.
///     - 2009.10.10 Add search priority in KeywordSearchActionItem.
///

#ifndef _ACTIONITEM_H_
#define _ACTIONITEM_H_

#include <common/type_defs.h>

#include "QueryTypeDef.h"
#include "ConditionInfo.h"

#include <ranking-manager/RankingEnumerator.h>

#include <util/izene_serialization.h>
#include <sstream>
#include <vector>

namespace sf1r {

#ifndef USE_PROTOCOL_BUFFER

/**************************
 
///
/// @brief This class is a tree structure which consists of parsed keywords and operators of query string
///
class QueryTree
{
    public:
        QueryTree(void);
        ~QueryTree(void);

        QueryTree& operator=(const QueryTree& ob);

        void clear(void);

        void getQueryTermIdSetMap(sf1r::QTISM_T& queryTermIdSetMap);

    public:
        ///
        /// @brief This pointer is used as a PropertyExpr List. 
        ///        Some PropertyExpr class can have child Property Expr.
        ///
        boost::shared_ptr<PropertyExpr> propertyExpr_;

        ///
        /// @brief Flag if propertyExpr_ is released or not.
        ///
        bool bReleaseMemory_;
}; // end - class  QueryTree

***********************************/

///
/// @brief This class contains several information which is needed to display property.
///        One DisplayProperty contains all options of one specific property.
///
class DisplayProperty
{
    public:
        DisplayProperty() :
            isSnippetOn_(false),
            isSummaryOn_(false),
            summarySentenceNum_(0),
            isHighlightOn_(false) {};

        DisplayProperty(const std::string& obj) :
            isSnippetOn_(false),
            isSummaryOn_(false),
            summarySentenceNum_(0),
            summaryPropertyAlias_(),
            isHighlightOn_(false),
            propertyString_(obj) {};


        void print(std::ostream& out = std::cout) const 
        {
            stringstream ss;
            ss << endl;
            ss << "Class DisplayProperty" << endl;
            ss << "---------------------------------" << endl;
            ss << "Property : " << propertyString_ << endl;
            ss << "isHighlightOn_ : " << isHighlightOn_ << endl;
            ss << "isSnippetOn_   : " << isSnippetOn_   << endl;
            ss << "isSummaryOn_   : " << isSummaryOn_   << endl;
            ss << "Summary Sentence Number : " << summarySentenceNum_ << endl;
            ss << "Summary Property Alias : " << summaryPropertyAlias_ << endl;

            out << ss.str();
        }

        ///
        /// @brief a flag variable if snippet option is turned on in the query.
        ///
        bool isSnippetOn_;

        ///
        /// @brief a flag variable if summary option is turned on in the query.
        ///
        bool isSummaryOn_;

        ///
        /// @brief the number of summary sentences. It only affects if isSummaryOn_ is true.
        ///
        size_t summarySentenceNum_;

        /// @brief Summary is returned as another property. This field
        /// customized the name of that property. If it is empty, the name is
        /// determined by application.
        std::string summaryPropertyAlias_;

        ///
        /// @brief a flag variable if highlight option is turned on in the query.
        ///
        bool isHighlightOn_;

        ///
        /// @brief analyzed query string of specific property
        ///
        std::string propertyString_;

        DATA_IO_LOAD_SAVE(DisplayProperty, &isSnippetOn_&isSummaryOn_&summarySentenceNum_&summaryPropertyAlias_&isHighlightOn_&propertyString_);

    private:
        friend class boost::serialization::access;
        template<class Archive>
            void serialize(Archive& ar, const unsigned int version)
            {
                ar & isSnippetOn_;
                ar & isSummaryOn_;
                ar & summarySentenceNum_;
                ar & summaryPropertyAlias_;
                ar & isHighlightOn_;
                ar & propertyString_;
            }
}; // end - class DisplayProperty

inline bool operator==( const DisplayProperty& a, const DisplayProperty& b)
{
    return a.isSnippetOn_ == b.isSnippetOn_
        && a.isSummaryOn_ == b.isSummaryOn_
        && a.summarySentenceNum_ == b.summarySentenceNum_
        && a.isHighlightOn_ == b.isHighlightOn_
        && a.propertyString_ == b.propertyString_
        && a.summaryPropertyAlias_ == b.summaryPropertyAlias_;
} // end - operator==()

///
/// @brief This class is used for grouping option in the keyword search query.
///        One Grouping Option class has one grouping rule and it targets one property.
///
class GroupingOption
{
    public:
        GroupingOption(): num_(0) {};

        bool operator== (const GroupingOption& obj) const
        {
            return propertyString_ == obj.propertyString_
                && from_ == obj.from_
                && to_ == obj.to_
                && num_ == obj.num_;
        } // end - oerator==

        void print(ostream& out = std::cout) const
        {
            stringstream ss;
            ss << "GroupOption" << endl;
            ss << "\tPropertyString_ : " << propertyString_ << endl;
            ss << "\tfrom_ : " << from_ << endl;
            ss << "\tto_ : " << to_ << endl;
            ss << "\tnum_ : " << num_ << endl;
            out << ss.str();
        } // end - print()

        ///
        /// @grief a Name string of grouping target property.
        ///
        std::string   propertyString_;

        ///
        /// @brief one of the grouping range value. 
        ///        Property value can be selected if its value is greater than from_.
        ///
        PropertyValue from_;

        ///
        /// @brief one of the grouping range value. 
        ///        Property value can be selected if its value is less than to_.
        ///
        PropertyValue to_;

        ///
        /// @brief The limit number of documents of the group.
        ///
        size_t        num_;

        DATA_IO_LOAD_SAVE( GroupingOption, &propertyString_&from_&to_&num_);

    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar & propertyString_;
            ar & from_;
            ar & to_;
            ar & num_;
        }
}; // end - GroupingOption



///
/// @brief This class contains environment values of query requester. 
///        It is stored in most of the ActionItems.
///
class RequesterEnvironment
{
    public:

        RequesterEnvironment() : isLogging_(false) {};

        void print(std::ostream& out = std::cout) const
        {
            stringstream ss;
            ss << endl;
            ss << "RequesterEnvironment"                    << endl;
            ss << "------------------------------"          << endl;
            ss << "isLogging        : " << isLogging_       << endl;
            ss << "encodingType_    : " << encodingType_    << endl;
            ss << "queryString_     : " << queryString_     << endl;
            ss << "taxonomyLabel_   : " << taxonomyLabel_   << endl;
            ss << "nameEntityItem_  : " << nameEntityItem_  << endl;
            ss << "nameEntityType_  : " << nameEntityType_  << endl;
            ss << "ipAddress_       : " << ipAddress_       << endl;
            out << ss.str();
        }

        ///
        /// @brief a flag if current query should be logged.
        ///
        bool            isLogging_;

        ///
        /// @brief encoding type string.
        ///
        std::string     encodingType_;

        ///
        /// @brief a query string. The encoding type of query string 
        ///        and result XML will follow encodingType_.
        ///
        std::string     queryString_;

        ///
        /// @brief a string value of selected taxonomy label. It is used
        ///        only in Label Click Query.
        ///
        std::string     taxonomyLabel_;

        /// @brief Name entity item name
        std::string nameEntityItem_;

        /// @brief Name entity item type
        std::string nameEntityType_;

        ///
        /// @brief ip address of requester.
        ///
        std::string     ipAddress_;

        DATA_IO_LOAD_SAVE(RequesterEnvironment, 
                &isLogging_&encodingType_&queryString_&taxonomyLabel_
                &nameEntityItem_&nameEntityType_&ipAddress_);

    private:
        // Log : 2009.09.08
        // ---------------------------------------
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar & isLogging_;
            ar & encodingType_;
            ar & queryString_;
            ar & taxonomyLabel_;
            ar & nameEntityItem_;
            ar & nameEntityType_;
            ar & ipAddress_;
        } 
}; // end - queryEnvironment

inline bool operator==(
    const RequesterEnvironment& a,
    const RequesterEnvironment& b
)
{
    return a.isLogging_ == b.isLogging_
        && a.encodingType_ == b.encodingType_
        && a.queryString_ == b.queryString_
        && a.taxonomyLabel_ == b.taxonomyLabel_
        && a.ipAddress_ == b.ipAddress_;
}

///
/// @brief This class stores auto fill list information.
///
class AutoFillActionItem
{
    public:

        ///
        /// @brief environment value of requester
        /// @see RequesterEnvironment
        ///
        RequesterEnvironment        env_;

        ///
        /// @brief an auto fill string list which is stored with recommended terms by analyzing the queryString.
        ///
        std::vector<izenelib::util::UString>    autoFillList_;

        DATA_IO_LOAD_SAVE(AutoFillActionItem, &env_&autoFillList_)
}; // end - class AutoFillActionItem

///
/// @brief This class has information how to process when User starts indexing.
///
class IndexCommandActionItem
{
    public:
        ///
        /// @brief an environment variable of requester.
        /// @see RequesterEnvironment
        ///
        RequesterEnvironment            env_;

        ///
        /// @Number of documents which are going to be indexed.
        ///
        uint32_t                        numOfDocs_;

        ///
        /// @brief a list of collection name to be indexed.
        ///
        std::vector<std::string>        collectionNameList_;

        DATA_IO_LOAD_SAVE(IndexCommandActionItem, &env_&numOfDocs_&collectionNameList_)
}; // end - class IndexCommandActionItem



///
/// @brief This class has information how to process when User optimizes indexing.
///
class OptimizeIndexCommandActionItem
{
    public:
        ///
        /// @brief an environment variable of requester.
        /// @see RequesterEnvironment
        ///
        RequesterEnvironment            env_;

        ///
        /// @brief a list of collection name to be optimized.
        ///
        std::vector<std::string>        collectionNameList_;

        DATA_IO_LOAD_SAVE(OptimizeIndexCommandActionItem, &env_&collectionNameList_)
}; // end - class IndexCommandActionItem


///
/// @brief This class has information to process keyword searching.
///
class KeywordSearchActionItem
{

    public:
        // property name - sorting order(true : Ascending, false : Descending)
        typedef std::pair<std::string , bool> SortPriorityType; 
        // Filter Option - propertyName

        KeywordSearchActionItem() :
            removeDuplicatedDocs_(false)
        {
        }

        KeywordSearchActionItem(const KeywordSearchActionItem& obj)
            :env_(obj.env_),
            refinedQueryString_(obj.refinedQueryString_),
            collectionName_(obj.collectionName_),
            rankingType_(obj.rankingType_),
            pageInfo_(obj.pageInfo_),
            languageAnalyzerInfo_(obj.languageAnalyzerInfo_),
            searchPropertyList_(obj.searchPropertyList_),
            removeDuplicatedDocs_(obj.removeDuplicatedDocs_),
            displayPropertyList_(obj.displayPropertyList_),
            sortPriorityList_   (obj.sortPriorityList_),
            filteringList_(obj.filteringList_),
            groupingList_(obj.groupingList_) {}


        KeywordSearchActionItem& operator=(const KeywordSearchActionItem& obj)
        {
            env_ = obj.env_;
            refinedQueryString_ = obj.refinedQueryString_;
            collectionName_ = obj.collectionName_;
            rankingType_ = obj.rankingType_;
            pageInfo_ = obj.pageInfo_;
            languageAnalyzerInfo_ = obj.languageAnalyzerInfo_;
            searchPropertyList_ = obj.searchPropertyList_;
            removeDuplicatedDocs_ = obj.removeDuplicatedDocs_;
            displayPropertyList_ = obj.displayPropertyList_;
            sortPriorityList_    = obj.sortPriorityList_;
            filteringList_ = obj.filteringList_;
            groupingList_ = obj.groupingList_;

            return (*this);
        }

        bool operator==(const KeywordSearchActionItem& obj) const
        {
            return env_ == obj.env_
                && collectionName_ == obj.collectionName_
                && refinedQueryString_ == obj.refinedQueryString_
                && rankingType_ == obj.rankingType_
                && pageInfo_ == obj.pageInfo_
                && languageAnalyzerInfo_ == obj.languageAnalyzerInfo_
                && searchPropertyList_ == obj.searchPropertyList_
                && removeDuplicatedDocs_ == obj.removeDuplicatedDocs_
                && displayPropertyList_ == obj.displayPropertyList_
                && sortPriorityList_    == obj.sortPriorityList_
                && filteringList_ == obj.filteringList_
                && groupingList_ == obj.groupingList_;
        }

        void print(std::ostream& out = std::cout) const
        {
            stringstream ss;
            ss << endl;
            ss << "Class KeywordSearchActionItem" << endl;
            ss << "-----------------------------------" << endl;
            ss << "Collection Name  : " << collectionName_ << endl;
            ss << "Refined Query    : " << refinedQueryString_ << endl;
            ss << "RankingType      : " << rankingType_ << endl;
            ss << "PageInfo         : " << pageInfo_.start_ << " , " << pageInfo_.count_ << endl;
            ss << "LanguageAnalyzer : " << languageAnalyzerInfo_.applyLA_ << " , "
                                        << languageAnalyzerInfo_.useOriginalKeyword_ << " , "
                                        << languageAnalyzerInfo_.synonymExtension_ << endl;
            ss << "Search Property  : ";
            for(size_t i = 0; i < searchPropertyList_.size(); i++)
                ss << searchPropertyList_[i] << " ";
            ss << endl;
            ss << "removeDuplicate  : " << removeDuplicatedDocs_ << endl;
            env_.print(out);
            for(size_t i = 0; i < displayPropertyList_.size(); i++)
                displayPropertyList_[i].print(out);

            ss << "Sort Priority : " << endl;
            for(size_t i = 0; i < sortPriorityList_.size(); i++)
                ss << " - " << sortPriorityList_[i].first << " " << sortPriorityList_[i].second << endl;

            ss << "Filtering Option : " << endl;
            for(size_t i = 0; i < filteringList_.size(); i++)
            {
                ss << "FilteringType :  " << filteringList_[i].first.first << " , property : " << filteringList_[i].first.second << endl;
                ss << "------------------------------------------------" << endl;
                for( std::vector<PropertyValue>::const_iterator iter = filteringList_[i].second.begin();
                        iter != filteringList_[i].second.end(); iter++ )
                    ss << *iter << endl;
                ss << "------------------------------------------------" << endl;
            }
            ss << endl << "Grouping Option" << endl;
            ss << "------------------------------------------------" << endl;
            for(size_t i = 0; i < groupingList_.size(); i++)
            {
                groupingList_[i].print( ss );
            }
            ss << "------------------------------------------------" << endl;

            out << ss.str();
        }

        ///
        /// @brief an environment variable of requester.
        /// @see RequesterEnvironment
        ///
        RequesterEnvironment            env_;

        ///
        /// @brief a string of corrected query. It will be filled by query-correction-sub-manager
        ///        if it needs to be modified.
        ///
        izenelib::util::UString                     refinedQueryString_;

        ///
        /// @brief a collection name.
        ///
        std::string                     collectionName_;

        ///
        /// @brief ranking type of the query. BM25, KL and PLM can be used.
        ///
        RankingType::TextRankingType    rankingType_;

        ///
        /// @brief page information of current result page.
        /// @see PageInfo
        ///
        PageInfo                        pageInfo_;

        ///
        /// @brief This contains how to analyze input query string.
        ///
        LanguageAnalyzerInfo            languageAnalyzerInfo_;

        ///
        /// @brief a list of properties which are the target of searching.
        ///
        std::vector<std::string>        searchPropertyList_;

        ///
        /// @brief a flag if duplicated documents are removed in the result.
        ///
        bool                            removeDuplicatedDocs_;

        ///
        /// @brief a list of property which are the target of display in the result.
        ///
        std::vector<DisplayProperty>    displayPropertyList_;

        ///
        /// @brief a list of sort methods. The less the index of a sort item in  the list, the more it has priority.
        ///
        std::vector<SortPriorityType>   sortPriorityList_;

        ///
        /// @brief a list of filtering option.
        ///
        std::vector<QueryFiltering::FilteringType>      filteringList_;

        ///
        /// @brief a list of grouping option.
        ///
        std::vector<GroupingOption>     groupingList_;

        ///
        /// @brief a list of property query terms.
        ///

        DATA_IO_LOAD_SAVE(KeywordSearchActionItem, &env_&refinedQueryString_&collectionName_
                &rankingType_&pageInfo_&languageAnalyzerInfo_&searchPropertyList_&removeDuplicatedDocs_
                &displayPropertyList_&sortPriorityList_&filteringList_&groupingList_);

    private:
        
        // Log : 2009.09.08
        // ---------------------------------------
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar & env_;
            ar & refinedQueryString_;
            ar & collectionName_;
            ar & rankingType_;
            ar & pageInfo_;
            ar & languageAnalyzerInfo_;
            ar & searchPropertyList_;
            ar & removeDuplicatedDocs_;
            ar & displayPropertyList_;
            ar & sortPriorityList_;
            ar & filteringList_;
            ar & groupingList_;
        }

}; // end - class KeywordSearchActionItem

/// @brief Information to process documents get request.
///
/// Get document by internal IDs or user specified DOCID's.
class GetDocumentsByIdsActionItem
{
    public:
        /// @brief an environment value of requester.
        RequesterEnvironment env_;

        /// @brief This contains how to analyze input query string.
        LanguageAnalyzerInfo languageAnalyzerInfo_;

        /// @brief Collection name.
        std::string collectionName_;

        /// @brief List of display properties which is shown in the result.
        std::vector<DisplayProperty> displayPropertyList_;

        /// @brief Internal document id list.
        std::vector<docid_t> idList_;

        /// @brief User specified DOCID list.
        std::vector<std::string> docIdList_;

        /// @brief filtering options
        std::vector<QueryFiltering::FilteringType> filteringList_;

        DATA_IO_LOAD_SAVE(
            GetDocumentsByIdsActionItem,
            &env_&languageAnalyzerInfo_&collectionName_&displayPropertyList_
            &idList_&docIdList_&filteringList_
        )

    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar &env_&languageAnalyzerInfo_&collectionName_&displayPropertyList_
                &idList_&docIdList_&filteringList_;
        }
}; // end - class GetDocumentsByIdsActionItem

///
/// @brief This class has information to start mining.
///
class MiningCommandActionItem
{
    public:

        ///
        /// @brief an environment variable of requester.
        ///
        RequesterEnvironment            env_;

        ///
        /// @brief the number of documents to be mining.
        ///
        size_t                          numOfDocs_;
	        
        ///
        /// @brief a list of collection name which should be mined.
        ///
        std::vector<std::string>        collectionNameList_;

        DATA_IO_LOAD_SAVE(MiningCommandActionItem, &env_&numOfDocs_&collectionNameList_)

}; // end - class MiningCommandActionItem

///
/// @brief this class contains information to process Recent Keyword Query.
///
class RecentKeywordActionItem
{
    public:
        ///
        /// @brief an environment variable of requster.
        ///
        RequesterEnvironment            env_;

        ///
        /// @brief this object contains two information. one is a target collection name.
        ///        And the other name is the number of requested recent keyword list.
        ///
        std::vector< std::pair<std::string , size_t> > collectionNameKeywordNoList_;

        ///
        /// @brief a list of recent keyword list.
        /// @details
        /// First index means collection index which is the same order of 
        /// collectionNameKeywordNoList_
        ///
        std::vector< std::vector<izenelib::util::UString> > recentKeywordList_;

        DATA_IO_LOAD_SAVE(RecentKeywordActionItem, &env_&collectionNameKeywordNoList_&recentKeywordList_);
}; // end - class RecentKeywordActionItem

#else

#endif

/**************************************************
///
/// @brief This class stores Query Tree information.
///
class SearchKeywordOperation
{
    public:
        ///
        /// @brief this class stores basic query information.
        ///
        KeywordSearchActionItem& actionItem_;

    private:
        ///
        /// @brief a query tree structure of current target.
        ///
        QueryTree queryTree_;

        ///
        /// @brief a list of query tree which includes following list.
        ///     - Original Query Tree
        ///     - Extended Query Tree : This is LA applied version of "Original Query Tree"
        ///     - Property Query Tree
        ///     - Extended Query Tree : This is LA applied version of "Property Query Tree"
        ///
        QueryTree queryTreeList_[4];

        ///
        /// @brief a list of flags if current query tree is available.
        ///
        bool queryTreeFlagList_[4];

        ///
        /// @brief a list of search property.
        ///
        std::vector<std::string> searchPropertyList_;

        ///
        /// @brief a map of information of property query terms.
        /// @details
        ///     - map->first   : property string
        ///     - map-->second : PropertyTermInfo object.
        ///
        std::map<std::string, PropertyTermInfo> propertyTermInfoMap_;

        ///
        /// @brief a list of query terms which is not applied Language Analyzer.
        ///
        std::vector<termid_t> originalTermIdList_;

        ///
        /// @brief a list of boolean operattion based Property Expr objects.
        ///
        std::vector<boost::shared_ptr<BoolPropertyExpr> > boolPropertyList_;

        ///
        /// @brief a list of wildcard term id list.
        ///
        std::vector<termid_t> wildcardTermIdList_;

        ///
        /// @brief a map of search property - set<termid_t>
        ///
        sf1r::QTISM_T queryTermIdSetMap_;

    public:
        enum QueryTreeType
        {
            QUERY_TREE = 0,
            EXTENDED_QUERY_TREE,
            PROPERTY_QUERY_TREE,
            EXTENDED_PROPERTY_QUERY_TREE
        };

        SearchKeywordOperation(KeywordSearchActionItem& actionItem);

        ~SearchKeywordOperation();

        ///
        /// @brief clear member variables
        /// @return void
        ///
        void clear(void);

        ///
        /// @brief This function copies given QueryTree object into proper place in queryTreeList_
        /// @param[queryTreeType]   The Type of query tree.
        /// @param[queryTree]       source query tree.
        ///
        void setQueryTree(QueryTreeType queryTreeType, const QueryTree& queryTree);

        ///
        /// @brief This function extracts query tree object from queryTreeList_ to queryTree_.
        ///
        void mergeQueryTree(void);

        ///
        /// @brief This function gets queryTree_
        /// @param[queryTree]   target object of copying query tree.
        ///
        void getQueryTree(QueryTree& queryTree) const;

        ///
        /// @brief Sets a list of search properties.
        /// @param[propertyList]    a list of search properties.
        ///
        void setSearchPropertyList(const std::vector<std::string>& propertyList);

        ///
        /// @brief Gets a list of search properties.
        /// @param[propertyList]    an output list of search properties.
        ///
        void getSearchPropertyList(std::vector<std::string>& propertyList) const;

        ///
        /// @brief Returns a list of search properties.
        /// @return a list of search properties.
        ///
        const std::vector<std::string>& getSearchPropertyList() const
        {
            return searchPropertyList_;
        }

        ///
        /// @brief This function return LanguageAnalyzerInfo object
        /// @return LanguageAnalyzerInfo object
        ///
        const LanguageAnalyzerInfo& getLanguageAnalyzerInfo(void) const;

        ///
        /// @brief build PropertyTermInfo list
        /// @param[queryTree]   query tree
        /// @return true  - Success to process.
        /// @return false - Fail    to process.
        ///
        bool buildPropertyTermInfoMap(const QueryTree& queryTree);

        ///
        /// @brief return PropertyTermInfo map
        /// @return propertyTermInfoMap_
        ///
        const std::map<std::string, PropertyTermInfo>& getPropertyTermInfoMap(void) const;

        ///
        /// @brief find PropertyTermInfo for given property.
        /// @param[searchProperty]      search property name
        /// @param[propertyTermInfo]    output of propertyTermInfo
        /// @return true - success to process, false - function failed.
        ///
        bool getPropertyTermInfo(const std::string& searchProperty, PropertyTermInfo& propertyTermInfo);

        ///
        /// @brief return vector of original term id
        /// @return reference of originalTermIdList_ variable
        ///
        std::vector<termid_t>& getOriginalTermIdList(void);

        ///
        /// @brief sets original term id list.
        /// @param[termIdList] a list of original term id.
        ///
        void setOriginalTermIdList(const std::vector<termid_t>& termIdList);

        ///
        /// @brief returns bool property list.
        /// @return a list of bool property expr pointer.
        ///
        std::vector<boost::shared_ptr<BoolPropertyExpr> >& getBoolPropertyList(void);

        ///
        /// @brief set wildcard termIdList.
        ///
        void setWildcardTermIdList(const std::vector<termid_t>& termIdList);
        
        ///
        /// @brief get wildcard termIdList.
        ///
        void getWildcardTermIdList(std::vector<termid_t>& termIdList) const;

        ///
        /// @brief set query term id set map.
        ///
        void setQueryTermIdSetMap(const sf1r::QTISM_T& queryTermIdSetMap);

        ///
        /// @brief get query term id set map.
        ///
        void getQueryTermIdSetMap(sf1r::QTISM_T& queryTermIdSetMap);

        ///
        /// @brief get query term id set of given search property.
        /// @param[searchProperty] a property name which is searched from queryTermIdSetMap_.
        /// @param[termIdSet] an output of this interface which contains term id set of given property.
        /// @return true if success, or false.
        ///
        bool getQueryTermIdSetByProperty(const std::string& searchProperty, std::set<sf1r::termid_t>& termIdSet);

        ///
        /// @brief sort brother nodes of boolean query tree recursively.
        /// @return true if success. false is for none-boolean type QueryTree or unexpected fail.
        ///
        bool sortBoolQueryTree();

}; // end - class SearchKeywordOperation
************************************************/
} // end - namespace sf1r

#endif // _ACTION_ITEM_
