/**
 * @file DistributeResultType.h
 * @author Zhongxia Li
 * @date Aug 12, 2011
 * @brief 
 */
#ifndef DISTIBUTE_RESULT_TYPE_H_
#define DISTIBUTE_RESULT_TYPE_H_

#include "ResultType.h"

//#include "type_defs.h"

namespace sf1r{

class KeywordPreSearchResult
{
public:

	KeywordPreSearchResult() : preResultType_(RESULT_TYPE_NOACTION) {}

	static const int RESULT_TYPE_NOACTION = 0x0;
	static const int RESULT_TYPE_FECTH = 0x1;
	static const int RESULT_TYPE_SEND = 0x2;

	/// @brief indicate how this result is used.
	int preResultType_;

    /// @brief document frequency info of terms for each property
    DocumentFrequencyInProperties dfmap_;

    /// @brief collection term frequency info of terms for each property
    CollectionTermFrequencyInProperties ctfmap_;

    MSGPACK_DEFINE();
};

class KeywordRealSearchResult : public ResultBase
{
public:
	KeywordRealSearchResult()
    : preSearchResult_(), rawQueryString_(""), encodingType_(izenelib::util::UString::UTF_8)
    , collectionName_(""), analyzedQuery_(), queryTermIdList_()
    , totalCount_(0), topKDocs_(0), topKRankScoreList_(0), topKCustomRankScoreList_(0)
    , start_(0), count_(0), onto_rep_()
    {
    }

	/// pre-fetched info for keyword search
	KeywordPreSearchResult preSearchResult_;

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

    /// For results in page in one node, indicates corresponding postions in topk results overall nodes.
    std::vector<size_t> topKPostionList_;

    /// property query terms
    std::vector<std::vector<izenelib::util::UString> > propertyQueryTermList_;


    sf1r::faceted::OntologyRep onto_rep_;

    // a list, each element is a label tree for a property
    sf1r::faceted::OntologyRep groupRep_;

    // a list, each element is a label array for an attribute name
    sf1r::faceted::OntologyRep attrRep_;

    MSGPACK_DEFINE(
    		/*preSearchResult_,*/rawQueryString_,encodingType_,collectionName_,analyzedQuery_,
            queryTermIdList_,totalCount_,topKDocs_,topKWorkerIds_,topKRankScoreList_,
            topKCustomRankScoreList_,start_,count_,topKPostionList_,propertyQueryTermList_,
            onto_rep_,groupRep_,attrRep_);
};

static void copySearchResult(KeywordSearchResult& resultDest, const KeywordRealSearchResult& resultSrc)
{
	//resultDest.rawQueryString_ = resultSrc.rawQueryString_;
	//resultDest.encodingType_ = resultSrc.rawQueryString_;
	//resultDest.collectionName_ = resultSrc.collectionName_;
	resultDest.analyzedQuery_ = resultSrc.analyzedQuery_;
	resultDest.queryTermIdList_ = resultSrc.queryTermIdList_;
	resultDest.totalCount_ = resultSrc.totalCount_;
	resultDest.topKDocs_ = resultSrc.topKDocs_;
	resultDest.topKWorkerIds_ = resultSrc.topKWorkerIds_;
	resultDest.topKRankScoreList_ = resultSrc.topKRankScoreList_;
	resultDest.topKCustomRankScoreList_ = resultSrc.topKCustomRankScoreList_;
	//resultDest.start_ = resultSrc.start_;
	//resultDest.count_ = resultSrc.count_;
	resultDest.topKPostionList_ = resultSrc.topKPostionList_;
	resultDest.propertyQueryTermList_ = resultSrc.propertyQueryTermList_;
	resultDest.onto_rep_ = resultSrc.onto_rep_;
	resultDest.groupRep_ = resultSrc.groupRep_;
	resultDest.attrRep_ = resultSrc.attrRep_;
}

}

#endif /* DISTIBUTE_RESULT_TYPE_H_ */
