///
/// @file   ResultType.h
/// @brief  Group of Result data types.
/// @author Dohyun Yun
/// @date   2009.08.12
/// @details
/// - Log
///     - 2009.08.27 Added DocumentClickResultFromMIA by Dohyun Yun
///     - 2009.08.31 Added RawTextResultFromSIA by Dohyun Yun
///     - 2009.12.02 Added ErrorInfo class by Dohyun Yun
///     - 2011.04.22 Added topKCustomRankScoreList_ in KeywordSearchResult by Zhongxia Li
///

#ifndef _RESULTTYPE_H_
#define _RESULTTYPE_H_

#include <common/type_defs.h>
#include <common/sf1_msgpack_serialization_types.h>
#include <query-manager/ConditionInfo.h>
#include <mining-manager/taxonomy-generation-submanager/TgTypes.h>
#include <mining-manager/faceted-submanager/ontology_rep.h>
#include <mining-manager/faceted-submanager/GroupRep.h>
#include <idmlib/concept-clustering/cc_types.h>
// #include <mining-manager/faceted-submanager/manmade_doc_category_item.h>
#include <util/ustring/UString.h>
#include <util/izene_serialization.h>

#include <3rdparty/msgpack/msgpack.hpp>
#include <net/aggregator/Util.h>

#include <sstream>
#include <vector>
#include <iterator>
#include <algorithm>

namespace sf1r
{

struct PropertyRange
{
    PropertyRange()
        : highValue_(0)
        , lowValue_(0)
    {}
    void swap(PropertyRange& range)
    {
        using std::swap;
        swap(highValue_, range.highValue_);
        swap(lowValue_, range.lowValue_);
    }
    float highValue_;
    float lowValue_;

    MSGPACK_DEFINE(highValue_, lowValue_);
};


///
/// @brief This class is inheritable type of every result type in this file.
///
class ErrorInfo
{
public:
    ErrorInfo() : errno_(0) {};
    ~ErrorInfo() {};

    ///
    /// @brief system defined error no.
    ///
    int         errno_;

    ///
    /// @brief detail error message of the error.
    ///
    std::string error_;

    DATA_IO_LOAD_SAVE(ErrorInfo, &errno_&error_);

    MSGPACK_DEFINE(errno_,error_);
}; // end - class ErrorInfo

typedef ErrorInfo ResultBase;

// Definition of divided search results
#include "DistributeResultType.h"

class KeywordSearchResult : public ErrorInfo
{
public:

    KeywordSearchResult()
        : encodingType_(izenelib::util::UString::UTF_8)
        , totalCount_(0), topKDocs_(0), topKRankScoreList_(0), topKCustomRankScoreList_(0)
        , start_(0), count_(0)
        , timeStamp_(0)
    {
    }

    void print(std::ostream& out = std::cout) const
    {
        stringstream ss;
        ss << endl;
        ss << "==== Class KeywordSearchResult ====" << endl;
        ss << "-----------------------------------" << endl;
        ss << "rawQueryString_    : " << rawQueryString_ << endl;
        ss << "encodingType_      : " << encodingType_ << endl;
        ss << "collectionName_    : " << collectionName_ << endl;
        ss << "analyzedQuery_     : " ;
        for (size_t i = 0; i < analyzedQuery_.size(); i ++)
        {
            string s;
            analyzedQuery_[i].convertString(s, izenelib::util::UString::UTF_8);
            ss << s << ", ";
        }
        ss << endl;
        ss << "queryTermIdList_   : " ;
        for (size_t i = 0; i < queryTermIdList_.size(); i ++)
        {
            ss << queryTermIdList_[i] << ", ";
        }
        ss << endl;
        ss << "totalCount_        : " << totalCount_ << endl;
        ss << "topKDocs_          : " << topKDocs_.size() << endl;
        for (size_t i = 0; i < topKDocs_.size(); i ++)
        {
            ss << "0x"<< hex << topKDocs_[i] << ", ";
        }
        ss << dec<< endl;
        ss << "topKWorkerIds_      : " << topKWorkerIds_.size() << endl;
        for (size_t i = 0; i < topKWorkerIds_.size(); i ++)
        {
            ss << topKWorkerIds_[i] << ", ";
        }
        ss << endl;
        std::vector<sf1r::wdocid_t> topKWDocs;
        const_cast<KeywordSearchResult*>(this)->getTopKWDocs(topKWDocs);
        ss << "topKWDocs          : " << topKWDocs.size() << endl;
        for (size_t i = 0; i < topKWDocs.size(); i ++)
        {
            ss << "0x" << hex<< topKWDocs[i] << ", ";
        }
        ss << dec<< endl;
        ss << "topKRankScoreList_      : " << topKRankScoreList_.size() << endl;
        for (size_t i = 0; i < topKRankScoreList_.size(); i ++)
        {
            ss << topKRankScoreList_[i] << ", ";
        }
        ss << endl;
        ss << "topKCustomRankScoreList_: " << endl;
        for (size_t i = 0; i < topKCustomRankScoreList_.size(); i ++)
        {
            ss << topKCustomRankScoreList_[i] << ", ";
        }
        ss << endl;
        ss << "page start_    : " << start_ << " count_   : " << count_ << endl;
        ss << endl;
        ss << "pageOffsetList_          : " << pageOffsetList_.size() << endl;
        for (size_t i = 0; i < pageOffsetList_.size(); i ++)
        {
            ss << pageOffsetList_[i] << ", ";
        }
        ss << endl << endl;

        ss << "propertyQueryTermList_ : " << endl;
        for (size_t i = 0; i < propertyQueryTermList_.size(); i++)
        {
            for (size_t j = 0; j < propertyQueryTermList_[i].size(); j++)
            {
                string s;
                propertyQueryTermList_[i][j].convertString(s, izenelib::util::UString::UTF_8);
                ss << s << ", ";
            }
            ss << endl;
        }
        ss << endl;

        ss << "fullTextOfDocumentInPage_      : " << fullTextOfDocumentInPage_.size() << endl;
        for (size_t i = 0; i < fullTextOfDocumentInPage_.size(); i++)
        {
            for (size_t j = 0; j < fullTextOfDocumentInPage_[i].size(); j++)
            {
            }
            ss << fullTextOfDocumentInPage_[i].size() << endl;
        }
        ss << endl;
        ss << "snippetTextOfDocumentInPage_   : " << snippetTextOfDocumentInPage_.size() << endl;
        for (size_t i = 0; i < snippetTextOfDocumentInPage_.size(); i++)
        {
            for (size_t j = 0; j < snippetTextOfDocumentInPage_[i].size(); j++)
            {
                //string s;
                //snippetTextOfDocumentInPage_[i][j].convertString(s, izenelib::util::UString::UTF_8);
                //ss << s << ", ";
            }
            ss << snippetTextOfDocumentInPage_[i].size() << endl;
        }
        ss << endl;
        ss << "rawTextOfSummaryInPage_        : " << rawTextOfSummaryInPage_.size() << endl;
        for (size_t i = 0; i < rawTextOfSummaryInPage_.size(); i++)
        {
            for (size_t j = 0; j < rawTextOfSummaryInPage_[i].size(); j++)
            {
            }
            ss << rawTextOfSummaryInPage_[i].size() << endl;
        }
        ss << endl;

        ss << "numberOfDuplicatedDocs_        : " << numberOfDuplicatedDocs_.size() << endl;
        for (size_t i = 0; i < numberOfDuplicatedDocs_.size(); i ++)
        {
            ss << numberOfDuplicatedDocs_[i] << ", ";
        }
        ss << endl;
        ss << "numberOfSimilarDocs_           : " << numberOfSimilarDocs_.size() << endl;
        for (size_t i = 0; i < numberOfSimilarDocs_.size(); i ++)
        {
            ss << numberOfSimilarDocs_[i] << ", ";
        }
        ss << endl;
        ss << "docCategories_          : " << endl;
        for (size_t i = 0; i < docCategories_.size(); i++)
        {
            for (size_t j = 0; j < docCategories_[i].size(); j++)
            {
                string s;
                docCategories_[i][j].convertString(s, izenelib::util::UString::UTF_8);
                ss << s << ", ";
            }
            ss << endl;
        }
        ss << endl;
        //               ss << "imgs_        : " << imgs_.size() << endl;
        //               for (size_t i = 0; i < imgs_.size(); i ++)
        //               {
        //                   ss << imgs_[i] << ", ";
        //               }
        //               ss << endl;

        ss << "taxonomyString_    : " << taxonomyString_.size() << endl;
        for (size_t i = 0; i < taxonomyString_.size(); i++)
        {
            string s;
            taxonomyString_[i].convertString(s, izenelib::util::UString::UTF_8);
            ss << s << ", ";
        }
        ss << endl;

        ss << "numOfTGDocs_    : " << numOfTGDocs_.size() << endl;
        for (size_t i = 0; i < numOfTGDocs_.size(); i++)
        {
            ss << numOfTGDocs_[i] << ", ";
        }
        ss << endl;

        ss << "onto_rep_ : " <<endl;
        ss << onto_rep_.ToString();
        ss << "groupRep_ : " <<endl;
        ss << groupRep_.ToString();
        ss << "attrRep_ : " <<endl;
        ss << attrRep_.ToString();

        ss << "Finish time : " << std::ctime(&timeStamp_) << endl;

        ss << "===================================" << endl;
        out << ss.str();
    }

    std::string rawQueryString_;

    /// Distributed search info
    DistKeywordSearchInfo distSearchInfo_;

    ///
    /// @brief encoding type of rawQueryString
    ///
    izenelib::util::UString::EncodingType encodingType_;

    std::string collectionName_;
    /// A list of analyzed query string.
    std::vector<izenelib::util::UString> analyzedQuery_;

    std::vector<termid_t> queryTermIdList_;

    /// Total number of result documents
    std::size_t totalCount_;

    /// A list of ranked docId. First docId gets high rank score.
    std::vector<docid_t> topKDocs_;

    /// A list of workerids. The sequence is following \c topKDocs_.
    std::vector<uint32_t> topKWorkerIds_;

    std::vector<uint32_t> topKtids_;

    /// A list of rank scores. The sequence is following \c topKDocs_.
    std::vector<float> topKRankScoreList_;

    /// A list of custom ranking scores. The sequence is following \c topKDocs_.
    std::vector<float> topKCustomRankScoreList_;

    PropertyRange propertyRange_;

    std::size_t start_;

    /// @brief number of documents in current page
    std::size_t count_;

    /// For results in page in one node, indicates corresponding postions in that page result.
    std::vector<size_t> pageOffsetList_;

    /// property query terms
    std::vector<std::vector<izenelib::util::UString> > propertyQueryTermList_;

    /// @brief Full text of documents in one page. It will be used for
    /// caching in BA when "DocumentClick" query occurs.
    /// @see pageInfo_
    ///
    /// @details
    /// ProtoType : fullTextOfDocumentInPage_[DISPLAY PROPERTY Order][DOC Order]
    /// - DISPLAY PROPERTY Order : The sequence which follows the order in displayPropertyList_
    /// - DOC Order : The sequence which follows the order in topKDocs_
    std::vector<std::vector<izenelib::util::UString> >  fullTextOfDocumentInPage_;

    /// @brief Displayed text of documents in one page.
    /// @see pageInfo_
    ///
    /// The first index corresponds to each property in \c
    /// KeywordSearchActionItem::displayPropertylist with the same
    /// order.
    ///
    /// The inner vector is raw text for documents in current page.
    std::vector<std::vector<izenelib::util::UString> >  snippetTextOfDocumentInPage_;

    /// @brief Summary of documents in one page.
    /// @see pageInfo_
    ///
    /// The first index corresponds to each property in \c
    /// KeywordSearchActionItem::displayPropertylist which is marked
    /// generating summary with the same order.
    /// The inner vector is summary text for documents in current page.
    //
    /// (ex) Display Property List : title, content, attach.
    ///      Summary Option On : title, attach.
    ///
    ///      size of rawTextOfSummaryInPage_ == 2
    ///      rawTextOfSummaryInPage_[0][1] -> summary of second document[1] of title property[0].
    ///      rawTextOfSummaryInPage_[1][3] -> summary of fourth document[3] of attach property[0].
    ///
    ///
    std::vector<std::vector<izenelib::util::UString> >  rawTextOfSummaryInPage_;


    std::vector<count_t> numberOfDuplicatedDocs_;

    std::vector<count_t> numberOfSimilarDocs_;

    std::vector<std::vector<izenelib::util::UString> > docCategories_;

    std::vector<uint32_t> imgs_;

    // --------------------------------[ Taxonomy List ]

    idmlib::cc::CCInput32 tg_input;

    /// Taxonomy string list.
    std::vector<izenelib::util::UString> taxonomyString_;

    /// A list which stores the number of documents which are related to the specific TG item.
    std::vector<count_t> numOfTGDocs_;

    /// A list which stores the id list of documents which are related to the specific TG item.
    std::vector<std::vector<uint64_t> > tgDocIdList_;

    /// TaxnomyLevel is used for displaying hierarchical taxonomy result. It will start from 0.
    ///
    /// (ex)
    /// <TgLabel name=”finantial” num=”2”>
    ///     <TgLabel name=”target2” num”3”>
    ///         <TgLabel name=”target3” num=”2”/>
    ///     </TgLabel>
    /// </TgLabel>
    /// <TgLabel name=”bank” num=”4”/>
    ///
    /// numOfTGDocs_ = { 0 , 1 , 2 , 0 };
    std::vector<uint32_t> taxonomyLevel_; // Start From 0

    NEResultList neList_;
    // --------------------------------[ Related Query ]

    sf1r::faceted::OntologyRep onto_rep_;

    // a list, each element is a label tree for a property
    sf1r::faceted::GroupRep groupRep_;

    // a list, each element is a label array for an attribute name
    sf1r::faceted::OntologyRep attrRep_;

    /// A list of related query string.
    std::deque<izenelib::util::UString> relatedQueryList_;

    /// A list of related query rank score.
    std::vector<float> rqScore_;

    /// Finish time of searching
    std::time_t timeStamp_;

    void getTopKWDocs(std::vector<sf1r::wdocid_t>& topKWDocs) const
    {
        if (topKWorkerIds_.size() <= 0)
        {
            topKWDocs.assign(topKDocs_.begin(), topKDocs_.end());
            return;
        }

        topKWDocs.resize(topKDocs_.size());
        for (size_t i = 0; i < topKDocs_.size(); i++)
        {
            topKWDocs[i] = net::aggregator::Util::GetWDocId(topKWorkerIds_[i], topKDocs_[i]);
        }
    }

    void setStartCount(const PageInfo& pageInfo)
    {
        start_ = pageInfo.start_;
        count_ = pageInfo.count_;
    }

    void adjustStartCount(std::size_t topKStart)
    {
        std::size_t topKEnd = topKStart + topKDocs_.size();

        if (start_ > topKEnd)
        {
            start_ = topKEnd;
        }

        if (start_ + count_ > topKEnd)
        {
            count_ = topKEnd - start_;
        }
    }

    void assign(const DistKeywordSearchResult& result)
    {
        rawQueryString_ = result.rawQueryString_;
        encodingType_ = result.encodingType_;
        collectionName_ = result.collectionName_;
        analyzedQuery_ = result.analyzedQuery_;
        queryTermIdList_ = result.queryTermIdList_;
        totalCount_ = result.totalCount_;
        topKDocs_ = result.topKDocs_;
        topKWorkerIds_ = result.topKWorkerIds_;
        topKtids_ = result.topKtids_;
        topKRankScoreList_ = result.topKRankScoreList_;
        topKCustomRankScoreList_ = result.topKCustomRankScoreList_;
        start_ = result.start_;
        count_ = result.count_;
        pageOffsetList_ = result.pageOffsetList_;
        propertyQueryTermList_ = result.propertyQueryTermList_;
        onto_rep_ = result.onto_rep_;
        groupRep_ = result.groupRep_;
        attrRep_ = result.attrRep_;
    }

    void swap(DistKeywordSearchResult& result)
    {
        rawQueryString_.swap(result.rawQueryString_);
        encodingType_ = result.encodingType_;
        collectionName_.swap(result.collectionName_);
        analyzedQuery_.swap(result.analyzedQuery_);
        queryTermIdList_.swap(result.queryTermIdList_);
        totalCount_ = result.totalCount_;
        topKDocs_.swap(result.topKDocs_);
        topKWorkerIds_.swap(result.topKWorkerIds_);
        topKtids_.swap(result.topKtids_);
        topKRankScoreList_.swap(result.topKRankScoreList_);
        topKCustomRankScoreList_.swap(result.topKCustomRankScoreList_);
        start_ = result.start_;
        count_ = result.count_;
        pageOffsetList_.swap(result.pageOffsetList_);
        propertyQueryTermList_.swap(result.propertyQueryTermList_);
        onto_rep_.swap(result.onto_rep_);
        groupRep_.swap(result.groupRep_);
        attrRep_.swap(result.attrRep_);
    }

    void swap(KeywordSearchResult& other)
    {
        using std::swap;
        rawQueryString_.swap(other.rawQueryString_);
        swap(encodingType_, other.encodingType_);
        collectionName_.swap(other.collectionName_);
        analyzedQuery_.swap(other.analyzedQuery_);
        queryTermIdList_.swap(other.queryTermIdList_);
        swap(totalCount_, other.totalCount_);
        topKDocs_.swap(other.topKDocs_);
        topKWorkerIds_.swap(other.topKWorkerIds_);
        topKtids_.swap(other.topKtids_);
        topKRankScoreList_.swap(other.topKRankScoreList_);
        topKCustomRankScoreList_.swap(other.topKCustomRankScoreList_);
        propertyRange_.swap(other.propertyRange_);
        swap(start_, other.start_);
        swap(count_, other.count_);
        pageOffsetList_.swap(other.pageOffsetList_);
        propertyQueryTermList_.swap(other.propertyQueryTermList_);
//      fullTextOfDocumentInPage_.swap(other.fullTextOfDocumentInPage_);
//      snippetTextOfDocumentInPage_.swap(other.snippetTextOfDocumentInPage_);
//      rawTextOfSummaryInPage_.swap(other.rawTextOfSummaryInPage_);
        numberOfDuplicatedDocs_.swap(other.numberOfDuplicatedDocs_);
        numberOfSimilarDocs_.swap(other.numberOfSimilarDocs_);
        docCategories_.swap(other.docCategories_);
        tg_input.swap(other.tg_input);
        taxonomyString_.swap(other.taxonomyString_);
        numOfTGDocs_.swap(other.numOfTGDocs_);
        taxonomyLevel_.swap(other.taxonomyLevel_);
        tgDocIdList_.swap(other.tgDocIdList_);
        neList_.swap(other.neList_);
        onto_rep_.swap(other.onto_rep_);
        groupRep_.swap(other.groupRep_);
        attrRep_.swap(other.attrRep_);
        relatedQueryList_.swap(other.relatedQueryList_);
        rqScore_.swap(other.rqScore_);
        swap(timeStamp_, other.timeStamp_);
    }

//  DATA_IO_LOAD_SAVE(KeywordSearchResult,
//          &rawQueryString_&encodingType_&collectionName_&analyzedQuery_
//          &queryTermIdList_&totalCount_
//          &topKDocs_&topKWorkerIds_&topKRankScoreList_&topKCustomRankScoreList_
//          &start_&count_&propertyQueryTermList_&fullTextOfDocumentInPage_
//          &snippetTextOfDocumentInPage_&rawTextOfSummaryInPage_
//          &errno_&error_
//          &numberOfDuplicatedDocs_&numberOfSimilarDocs_&docCategories_&imgs_&taxonomyString_&numOfTGDocs_&taxonomyLevel_&tgDocIdList_&neList_&onto_rep_&groupRep_&attrRep_&relatedQueryList_&rqScore_)

    MSGPACK_DEFINE(
            rawQueryString_, encodingType_, collectionName_, analyzedQuery_,
            queryTermIdList_, totalCount_, topKDocs_, topKWorkerIds_, topKtids_, topKRankScoreList_,
            topKCustomRankScoreList_, propertyRange_, start_, count_, pageOffsetList_, propertyQueryTermList_, fullTextOfDocumentInPage_,
            snippetTextOfDocumentInPage_, rawTextOfSummaryInPage_,
            numberOfDuplicatedDocs_, numberOfSimilarDocs_, docCategories_,
            tg_input, taxonomyString_, numOfTGDocs_, taxonomyLevel_, tgDocIdList_,
            neList_, onto_rep_, groupRep_, attrRep_, relatedQueryList_, rqScore_, timeStamp_);
};


class RawTextResultFromSIA : public ErrorInfo // Log : 2009.08.31
{
public:

    /// @brief Full text of documents in one page. It will be used for
    /// caching in BA when "DocumentClick" query occurs.
    /// @see pageInfo_
    ///
    /// @details
    /// ProtoType : fullTextOfDocumentInPage_[DISPLAY PROPERTY Order][DOC Order]
    /// - DISPLAY PROPERTY Order : The sequence which follows the order in displayPropertyList_
    /// - DOC Order : The sequence which follows the order of docIdList
    std::vector<std::vector<izenelib::util::UString> >  fullTextOfDocumentInPage_;

    /// Raw Text of Document is used for generating resultXML of "KeywordSearch" query.
    /// And the first index is following the sequence of the displayPropertyList in KeywordSearchActionItem.
    /// The second index is following the sequence of the rankedDocIdList_;
    //std::vector<std::vector<izenelib::util::UString> >  rawTextOfDocument_;
    std::vector<std::vector<izenelib::util::UString> >  snippetTextOfDocumentInPage_;


    /// Raw Text of Document is used for generating resultXML of "KeywordSearch" query.
    /// And the first index is following the sequence of the
    /// displayPropertyList(which is set to generate summary) in KeywordSearchActionItem.
    /// The second index is following the sequence of the rankedDocIdList_;
    //std::vector<std::vector<izenelib::util::UString> >  rawTextOfSummary_;
    std::vector<std::vector<izenelib::util::UString> >  rawTextOfSummaryInPage_;

    /// internal IDs of the documents
    std::vector<docid_t> idList_;

    /// corresponding workerids for each id. (no need to be serialized)
    std::vector<workerid_t> workeridList_;

    void getWIdList(std::vector<sf1r::wdocid_t>& widList) const
    {
        if (workeridList_.size() <= 0)
        {
            widList.assign(idList_.begin(), idList_.end());
            return;
        }

        widList.resize(idList_.size());
        for (size_t i = 0; i < idList_.size(); i++)
        {
            widList[i] = net::aggregator::Util::GetWDocId(workeridList_[i], idList_[i]);
        }
    }

    //LOG: changed the names for consistentcy with KeywordResultItem
    //DATA_IO_LOAD_SAVE(RawTextResultFromSIA,
    //        &fullTextOfDocumentInOnePage_&rawTextOfDocument_&rawTextOfSummary_

    DATA_IO_LOAD_SAVE(RawTextResultFromSIA,
            &fullTextOfDocumentInPage_&snippetTextOfDocumentInPage_&rawTextOfSummaryInPage_&idList_
            &errno_&error_
            );

    MSGPACK_DEFINE(fullTextOfDocumentInPage_, snippetTextOfDocumentInPage_, rawTextOfSummaryInPage_,
            idList_, errno_, error_);
}; // end - class RawTextResultFromSIA


class RawTextResultFromMIA : public RawTextResultFromSIA // Log : 2011.07.27
{
public:

    std::vector<count_t> numberOfDuplicatedDocs_;

    std::vector<count_t> numberOfSimilarDocs_;



    //LOG: changed the names for consistentcy with KeywordResultItem
    //DATA_IO_LOAD_SAVE(RawTextResultFromSIA,
    //        &fullTextOfDocumentInOnePage_&rawTextOfDocument_&rawTextOfSummary_

    DATA_IO_LOAD_SAVE(RawTextResultFromMIA,
            &fullTextOfDocumentInPage_&snippetTextOfDocumentInPage_&rawTextOfSummaryInPage_&idList_
            &numberOfDuplicatedDocs_&numberOfSimilarDocs_
            &errno_&error_
            );

    MSGPACK_DEFINE(fullTextOfDocumentInPage_, snippetTextOfDocumentInPage_, rawTextOfSummaryInPage_,
            idList_, numberOfDuplicatedDocs_, numberOfSimilarDocs_, errno_, error_);
}; // end - class RawTextResultFromMIA

/// @brief similar document id list and accompanying image id list.
class SimilarImageDocIdList
{
public:
    std::vector<uint32_t> imageIdList_;
    std::vector<docid_t>  docIdList_;

    void print(std::ostream& out = std::cout) const
    {
        std::stringstream ss;
        ss << std::endl;
        ss << "SimilarImageDocIdList class" << endl;
        ss << "-------------------------------------" << endl;
        ss << "imageIdList_ : ";
        std::copy(imageIdList_.begin(),
                imageIdList_.end(),
                std::ostream_iterator<uint32_t>(ss, " "));
        ss << std::endl;
        ss << "docIdList_ : ";
        std::copy(docIdList_.begin(),
                docIdList_.end(),
                std::ostream_iterator<docid_t>(ss, " "));
        ss << std::endl;
        out << ss.str();
    } // end - print()

    void clear(void)
    {
        imageIdList_.clear();
        docIdList_.clear();
    } // end - clear()

    DATA_IO_LOAD_SAVE(SimilarImageDocIdList,&imageIdList_&docIdList_);

    MSGPACK_DEFINE(imageIdList_, docIdList_);
};

} // end - namespace sf1r


#endif // _RESULTTYPE_H_
