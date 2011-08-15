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
///     - 2011.04.19 Add custom ranking information in KeywordSearchActionItem by Zhongxia Li
///

#ifndef _ACTIONITEM_H_
#define _ACTIONITEM_H_

#include <common/type_defs.h>

#include "QueryTypeDef.h"
#include "ConditionInfo.h"

#include <ranking-manager/RankingEnumerator.h>
#include <search-manager/CustomRanker.h>
#include <mining-manager/faceted-submanager/GroupParam.h>

#include <common/sf1_msgpack_serialization_types.h>
#include <3rdparty/msgpack/msgpack.hpp>
#include <util/izene_serialization.h>
#include <net/aggregator/Util.h>

#include <sstream>
#include <vector>
#include <utility>

#include <boost/shared_ptr.hpp>

namespace sf1r {

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

        MSGPACK_DEFINE(isSnippetOn_,isSummaryOn_,summarySentenceNum_,summaryPropertyAlias_,isHighlightOn_,propertyString_);

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
/// @brief This class contains environment values of query requester. 
///        It is stored in most of the ActionItems.
///
class RequesterEnvironment
{
    public:

        RequesterEnvironment() : isLogging_(false), isLogGroupLabels_(false) {};

        void print(std::ostream& out = std::cout) const
        {
            stringstream ss;
            ss << endl;
            ss << "RequesterEnvironment"                    << endl;
            ss << "------------------------------"          << endl;
            ss << "isLogging        : " << isLogging_       << endl;
            ss << "isLogGroupLabels_: " << isLogGroupLabels_<< endl;
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
        /// @brief a flag if current query should be logged.
        ///
        bool            isLogGroupLabels_;

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
        /// @brief the expanded query string.
        ///
        std::string expandedQueryString_;

        ///
        /// @brief a user id. The user id is accompanied with the query string
        ///          in a search request.
        ///
        std::string     userID_;

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
                &isLogging_&isLogGroupLabels_&encodingType_&queryString_&expandedQueryString_
                &userID_&taxonomyLabel_&nameEntityItem_&nameEntityType_&ipAddress_);

        MSGPACK_DEFINE(isLogging_,isLogGroupLabels_,encodingType_,queryString_,expandedQueryString_,
                userID_,taxonomyLabel_,nameEntityItem_,nameEntityItem_,nameEntityType_,ipAddress_);

    private:
        // Log : 2009.09.08
        // ---------------------------------------
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar & isLogging_;
            ar & isLogGroupLabels_;
            ar & encodingType_;
            ar & queryString_;
            ar & expandedQueryString_;
            ar & userID_;
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
        && a.isLogGroupLabels_ == b.isLogGroupLabels_
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

        MSGPACK_DEFINE(env_,numOfDocs_,collectionNameList_);
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

        MSGPACK_DEFINE(env_,collectionNameList_);
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
            groupParam_(obj.groupParam_),
            strExp_(obj.strExp_),
            paramConstValueMap_(obj.paramConstValueMap_),
            paramPropertyValueMap_(obj.paramPropertyValueMap_),
            customRanker_(obj.customRanker_) {}


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
            groupParam_ = obj.groupParam_;
            strExp_ = obj.strExp_;
            paramConstValueMap_ = obj.paramConstValueMap_;
            paramPropertyValueMap_ = obj.paramPropertyValueMap_;
            customRanker_ = obj.customRanker_;

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
                && groupParam_ == obj.groupParam_
                && strExp_ == obj.strExp_
                && paramConstValueMap_ == obj.paramConstValueMap_
                && paramPropertyValueMap_ == obj.paramPropertyValueMap_
                && customRanker_ == obj.customRanker_;
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
            ss << "------------------------------------------------" << endl;
            ss << groupParam_;
            ss << "------------------------------------------------" << endl;
            ss << endl << "Custom Ranking :" << endl;
            ss << "------------------------------------------------" << endl;
            ss << "\tExpression : " << strExp_ << endl;
            std::map<std::string, double>::const_iterator diter;
            for(diter = paramConstValueMap_.begin(); diter != paramConstValueMap_.end(); diter++)
            {
                ss << "\tParameter : " << diter->first << ", Value : " << diter->second << endl;
            }
            std::map<std::string, std::string>::const_iterator siter;
            for(siter = paramPropertyValueMap_.begin(); siter != paramPropertyValueMap_.end(); siter++)
            {
                ss << "\tParameter : " << siter->first << ", Value : " << siter->second << endl;
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
        izenelib::util::UString           refinedQueryString_;

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
        /// @brief group filter parameter
        ///
        faceted::GroupParam groupParam_;

        ///
        /// @brief a list of property query terms.
        ///

        ///
        /// @brief custom ranking information
        ///
        std::string strExp_;
        std::map<std::string, double> paramConstValueMap_;
        std::map<std::string, std::string> paramPropertyValueMap_;

        ///
        /// @brief custom ranking information(2)
        /// Avoid a second parsing by passing a reference to CustomRanker object.
        /// TODO, abandon this, serialization needed for remoted call
        boost::shared_ptr<CustomRanker> customRanker_;

        DATA_IO_LOAD_SAVE(KeywordSearchActionItem, &env_&refinedQueryString_&collectionName_
                &rankingType_&pageInfo_&languageAnalyzerInfo_&searchPropertyList_&removeDuplicatedDocs_
                &displayPropertyList_&sortPriorityList_&filteringList_&groupParam_
                &strExp_&paramConstValueMap_&paramPropertyValueMap_);

        /// msgpack serializtion
        MSGPACK_DEFINE(env_,refinedQueryString_,collectionName_,rankingType_,pageInfo_,languageAnalyzerInfo_,
                searchPropertyList_,removeDuplicatedDocs_,displayPropertyList_,sortPriorityList_,filteringList_,
                groupParam_,strExp_,paramConstValueMap_,paramPropertyValueMap_);

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
            ar & groupParam_;
            ar & strExp_;
            ar & paramConstValueMap_;
            ar & paramPropertyValueMap_;
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
        std::vector<wdocid_t> idList_;

        /// @brief User specified DOCID list.
        std::vector<std::string> docIdList_;

        /// @brief Property name, and values list
        std::string propertyName_;
        std::vector<PropertyValue> propertyValueList_;

        /// @brief filtering options
        std::vector<QueryFiltering::FilteringType> filteringList_;

        DATA_IO_LOAD_SAVE(
            GetDocumentsByIdsActionItem,
            &env_&languageAnalyzerInfo_&collectionName_&displayPropertyList_
            &idList_&docIdList_&filteringList_
        )

        MSGPACK_DEFINE(env_,languageAnalyzerInfo_,collectionName_,displayPropertyList_,
                idList_,docIdList_,propertyName_,propertyValueList_,filteringList_);

    public:
        std::set<sf1r::workerid_t> getDocWorkerIdLists(
                std::vector<sf1r::docid_t>& docidList,
                std::vector<sf1r::workerid_t>& workeridList)
        {
            std::set<sf1r::workerid_t> workerSet;
            for (size_t i = 0; i < idList_.size(); i++)
            {
                std::pair<sf1r::workerid_t, docid_t> wd = net::aggregator::Util::GetWorkerAndDocId(idList_[i]);
                workeridList.push_back(wd.first);
                docidList.push_back(wd.second);

                workerSet.insert(wd.first);
            }

            return workerSet;
        }

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
/// @brief Information for click group label.
///
class clickGroupLabelActionItem
{
public:
    clickGroupLabelActionItem() {}

    clickGroupLabelActionItem(
            const std::string& queryString,
            const std::string& propName,
            const std::string& propValue)
    :queryString_(queryString),
     propName_(propName),
     propValue_(propValue)
    {}

    std::string queryString_;

    std::string propName_;

    std::string propValue_;

    MSGPACK_DEFINE(queryString_, propName_, propValue_);
};

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

        MSGPACK_DEFINE(env_,numOfDocs_,collectionNameList_);

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

        MSGPACK_DEFINE(env_,collectionNameKeywordNoList_,recentKeywordList_);
}; // end - class RecentKeywordActionItem

} // end - namespace sf1r

#endif // _ACTION_ITEM_
