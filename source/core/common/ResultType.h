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
#include <mining-manager/taxonomy-generation-submanager/TgTypes.h>
#include <mining-manager/faceted-submanager/ontology_rep.h>
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
          , docCategories_()/*, imgs_()*/, taxonomyString_(), numOfTGDocs_()
          , tgDocIdList_(), taxonomyLevel_(), neList_(), onto_rep_()
          , relatedQueryList_(), rqScore_()
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
              ss << "queryTermIdList_   : " << endl;
              for (size_t i = 0; i < queryTermIdList_.size(); i ++)
              {
                  ss << queryTermIdList_[i] << ", ";
              }
              ss << endl;
              ss << "totalCount_        : " << totalCount_ << endl;
              ss << "topKDocs_          : " << topKDocs_.size() << endl;
              for (size_t i = 0; i < topKDocs_.size(); i ++)
              {
                  ss << "0x"<< hex<< topKDocs_[i] << ", ";
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
                  ss << "0x"<< hex<< topKWDocs[i] << ", ";
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
              ss << "topKPostionList_          : " << topKPostionList_.size() << endl;
              for (size_t i = 0; i < topKPostionList_.size(); i ++)
              {
                  ss << topKPostionList_[i] << ", ";
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

              ss << "===================================" << endl;
              out << ss.str();
          }

            void getTopKWDocs(std::vector<sf1r::wdocid_t>& topKWDocs)
            {
                if (topKWorkerIds_.size() == 0)
                    topKWorkerIds_.resize(topKDocs_.size(), 0);

                for (size_t i = 0; i < topKDocs_.size(); i++)
                {
                    topKWDocs.push_back(net::aggregator::Util::GetWDocId(topKWorkerIds_[i], topKDocs_[i]));
                }
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

            /// A list of workerids. The sequence is following \c topKDocs_.
            std::vector<uint32_t> topKWorkerIds_;

            /// A list of rank scores. The sequence is following \c topKDocs_.
            std::vector<float> topKRankScoreList_;

            /// A list of custom ranking scores. The sequence is following \c topKDocs_.
            std::vector<float> topKCustomRankScoreList_;

            std::size_t start_;

            /// @brief number of documents in current page
            std::size_t count_;

            /// For sub result, indecates the ordered postions in overall topk list.
            std::vector<size_t> topKPostionList_;

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

            std::vector< std::vector<izenelib::util::UString> > docCategories_;

             std::vector< uint32_t> imgs_;

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
            sf1r::faceted::OntologyRep groupRep_;

            // a list, each element is a label array for an attribute name
            sf1r::faceted::OntologyRep attrRep_;

            /// A list of related query string.
            std::deque<izenelib::util::UString> relatedQueryList_;

            /// A list of related query rank score.
            std::vector<float> rqScore_;
            
//             DATA_IO_LOAD_SAVE(KeywordSearchResult,
//                     &rawQueryString_&encodingType_&collectionName_&analyzedQuery_
//                     &queryTermIdList_&totalCount_
//                     &topKDocs_&topKWorkerIds_&topKRankScoreList_&topKCustomRankScoreList_
//                     &start_&count_&propertyQueryTermList_&fullTextOfDocumentInPage_
//                     &snippetTextOfDocumentInPage_&rawTextOfSummaryInPage_
//                     &errno_&error_
//                     &numberOfDuplicatedDocs_&numberOfSimilarDocs_&docCategories_&imgs_&taxonomyString_&numOfTGDocs_&taxonomyLevel_&tgDocIdList_&neList_&onto_rep_&groupRep_&attrRep_&relatedQueryList_&rqScore_)

            MSGPACK_DEFINE(
                    rawQueryString_,encodingType_,collectionName_,analyzedQuery_,
                    queryTermIdList_,totalCount_,topKDocs_,topKWorkerIds_,topKRankScoreList_,
                    topKCustomRankScoreList_,start_,count_,topKPostionList_,propertyQueryTermList_,fullTextOfDocumentInPage_,
                    snippetTextOfDocumentInPage_,rawTextOfSummaryInPage_,
                    errno_,error_,
                    numberOfDuplicatedDocs_,numberOfSimilarDocs_,docCategories_,
                    tg_input,taxonomyString_,numOfTGDocs_,taxonomyLevel_,tgDocIdList_,
                    neList_,onto_rep_,groupRep_,attrRep_,relatedQueryList_,rqScore_);
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
                    &fullTextOfDocumentInPage_&snippetTextOfDocumentInPage_&rawTextOfSummaryInPage_&idList_
                    &errno_&error_
                    );

            MSGPACK_DEFINE(fullTextOfDocumentInPage_,snippetTextOfDocumentInPage_,rawTextOfSummaryInPage_,
                    idList_,errno_,error_);
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

            MSGPACK_DEFINE(fullTextOfDocumentInPage_,snippetTextOfDocumentInPage_,rawTextOfSummaryInPage_,
                    idList_, numberOfDuplicatedDocs_, numberOfSimilarDocs_,errno_,error_);
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

            void clear(void) {
                imageIdList_.clear();
                docIdList_.clear();
            } // end - clear()

            DATA_IO_LOAD_SAVE(SimilarImageDocIdList,&imageIdList_&docIdList_);

            MSGPACK_DEFINE(imageIdList_, docIdList_);
    };

} // end - namespace sf1r


#endif // _RESULTTYPE_H_
