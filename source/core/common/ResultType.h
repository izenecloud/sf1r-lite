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

#include <mining-manager/taxonomy-generation-submanager/TgTypes.h>
#include <mining-manager/faceted-submanager/ontology_rep.h>
#include <idmlib/tdt/tdt_types.h>
#include <mining-manager/faceted-submanager/manmade_doc_category_item.h>
#include <util/ustring/UString.h>
#include <util/izene_serialization.h>
#include <3rdparty/msgpack/msgpack.hpp>

#include <sstream>
#include <vector>
#include <iterator>
#include <algorithm>

namespace sf1r {

    ///
    /// @brief This class is inheritable type of every result type in this file.
    ///
    class ErrorInfo {

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

    class KeywordSearchResult : public ErrorInfo
    {
        public:
          
          KeywordSearchResult()
          :rawQueryString_(""), encodingType_(izenelib::util::UString::UTF_8)
          , collectionName_(""), analyzedQuery_(), queryTermIdList_()
          , totalCount_(0), topKDocs_(0), topKRankScoreList_(0), topKCustomRankScoreList_(0)
          , start_(0), count_(0), fullTextOfDocumentInPage_()
          , snippetTextOfDocumentInPage_(), rawTextOfSummaryInPage_()
          , numberOfDuplicatedDocs_(), numberOfSimilarDocs_()
          , docCategories_(), imgs_(), taxonomyString_(), numOfTGDocs_()
          , tgDocIdList_(), taxonomyLevel_(), neList_(), onto_rep_()
          , relatedQueryList_(), rqScore_()
          {
          }
          
            std::string rawQueryString_;
            
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

            /// A list of rank scores. The sequence is following \c topKDocs_.
            std::vector<float> topKRankScoreList_;

            /// A list of custom ranking scores. The sequence is following \c topKDocs_.
            std::vector<float> topKCustomRankScoreList_;

            std::size_t start_;

            /// @brief number of documents in current page
            std::size_t count_;

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

            std::vector< std::vector<izenelib::util::UString> > docCategories_;

            std::vector< uint32_t> imgs_;

            // --------------------------------[ Taxonomy List ]

            /// Taxonomy string list.
            std::vector<izenelib::util::UString> taxonomyString_;

            /// A list which stores the number of documents which are related to the specific TG item.
            std::vector<count_t> numOfTGDocs_;

            /// A list which stores the id list of documents which are related to the specific TG item.
            std::vector<std::vector<docid_t> > tgDocIdList_;

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

            ne_result_list_type neList_;
            // --------------------------------[ Related Query ]
            
            sf1r::faceted::OntologyRep onto_rep_;

            // a list, each element is a label tree for a property
            sf1r::faceted::OntologyRep groupRep_;

            // a list, each element is a label array for an attribute name
            sf1r::faceted::OntologyRep attrRep_;

            /// A list of related query string.
            std::deque<izenelib::util::UString> relatedQueryList_;

            /// A list of related query rank score.
            std::vector<float> rqScore_;
            
            DATA_IO_LOAD_SAVE(KeywordSearchResult,
                    &rawQueryString_&encodingType_&collectionName_&analyzedQuery_
                    &queryTermIdList_&totalCount_
                    &topKDocs_&topKRankScoreList_&topKCustomRankScoreList_
                    &start_&count_&fullTextOfDocumentInPage_
                    &snippetTextOfDocumentInPage_&rawTextOfSummaryInPage_
                    &errno_&error_
                    &numberOfDuplicatedDocs_&numberOfSimilarDocs_&docCategories_&imgs_&taxonomyString_&numOfTGDocs_&taxonomyLevel_&tgDocIdList_&neList_&onto_rep_&groupRep_&attrRep_&relatedQueryList_&rqScore_)

            MSGPACK_DEFINE(
                    rawQueryString_,encodingType_,collectionName_,analyzedQuery_,
                    queryTermIdList_,totalCount_,topKDocs_,topKRankScoreList_,
                    topKCustomRankScoreList_,start_,count_,fullTextOfDocumentInPage_,
                    snippetTextOfDocumentInPage_,rawTextOfSummaryInPage_,errno_,error_,
                    numberOfDuplicatedDocs_,numberOfSimilarDocs_,docCategories_,imgs_,
                    taxonomyString_,numOfTGDocs_,taxonomyLevel_,tgDocIdList_,neList_,
                    onto_rep_,groupRep_,attrRep_,relatedQueryList_,rqScore_,errno_,error_);
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
            
            

            //LOG: changed the names for consistentcy with KeywordResultItem
            //DATA_IO_LOAD_SAVE(RawTextResultFromSIA, 
            //        &fullTextOfDocumentInOnePage_&rawTextOfDocument_&rawTextOfSummary_
            
            DATA_IO_LOAD_SAVE(RawTextResultFromSIA, 
                    &fullTextOfDocumentInPage_&snippetTextOfDocumentInPage_&rawTextOfSummaryInPage_
                    &errno_&error_
                    );

            MSGPACK_DEFINE(fullTextOfDocumentInPage_,snippetTextOfDocumentInPage_,rawTextOfSummaryInPage_,
                    idList_,errno_,error_);
    }; // end - class RawTextResultFromSIA

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

            void clear(void) {
                imageIdList_.clear();
                docIdList_.clear();
            } // end - clear()

            DATA_IO_LOAD_SAVE(SimilarImageDocIdList,&imageIdList_&docIdList_);

            MSGPACK_DEFINE(imageIdList_, docIdList_);
    };

} // end - namespace sf1r


#endif // _RESULTTYPE_H_
