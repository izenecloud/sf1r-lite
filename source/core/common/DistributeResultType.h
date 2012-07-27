/**
 * @file DistributeResultType.h
 * @author Zhongxia Li
 * @date Aug 12, 2011
 * @brief Distributed search result
 */

class DistKeywordSearchInfo
{
public:

    DistKeywordSearchInfo()
        : effective_(false)
        , option_(OPTION_NONE)
        , nodeType_(NODE_MASTER)
    {
    }

    static const int8_t OPTION_NONE = 0x0;
    static const int8_t OPTION_GATHER_INFO  = 0x1;
    static const int8_t OPTION_CARRIED_INFO = 0x2;

    static const int8_t NODE_MASTER = 0x0;
    static const int8_t NODE_WORKER = 0x1;

    /// @brief whether distributed search is effective
    bool effective_;

    /// @brief indicate .
    int8_t option_;
    int8_t nodeType_;

    /// @brief document frequency info of terms for each property
    DocumentFrequencyInProperties dfmap_;

    /// @brief collection term frequency info of terms for each property
    CollectionTermFrequencyInProperties ctfmap_;

    /// @brief maximal term frequency info of terms for each property
    MaxTermFrequencyInProperties maxtfmap_;

    /// @brief data lists for sort properties of docList.
    std::vector<std::pair<std::string , bool> > sortPropertyList_;
    std::vector<std::pair<std::string, std::vector<int32_t> > > sortPropertyInt32DataList_;
    std::vector<std::pair<std::string, std::vector<int64_t> > > sortPropertyInt64DataList_;
    std::vector<std::pair<std::string, std::vector<float> > > sortPropertyFloatDataList_;

    void swap(DistKeywordSearchInfo& other)
    {
        using std::swap;
        swap(effective_, other.effective_);
        swap(option_, other.option_);
        swap(nodeType_, other.nodeType_);
        dfmap_.swap(other.dfmap_);
        ctfmap_.swap(other.ctfmap_);
        maxtfmap_.swap(other.maxtfmap_);
        sortPropertyList_.swap(other.sortPropertyList_);
        sortPropertyInt32DataList_.swap(other.sortPropertyInt32DataList_);
        sortPropertyInt64DataList_.swap(other.sortPropertyInt64DataList_);
        sortPropertyFloatDataList_.swap(other.sortPropertyFloatDataList_);
    }

    MSGPACK_DEFINE(effective_, option_, nodeType_, dfmap_, ctfmap_, maxtfmap_, sortPropertyList_,
        sortPropertyInt32DataList_, sortPropertyInt64DataList_, sortPropertyFloatDataList_);
};

class DistKeywordSearchResult : public ErrorInfo
{
public:
    DistKeywordSearchResult()
        : encodingType_(izenelib::util::UString::UTF_8)
        , totalCount_(0), topKDocs_(0), topKRankScoreList_(0), topKCustomRankScoreList_(0)
        , start_(0), count_(0)
    {
    }

    void print(std::ostream& out = std::cout) const
    {
        stringstream ss;
        ss << endl;
        ss << "==== Class DistKeywordSearchResult ====" << endl;
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
            ss << "0x"<< hex<< topKDocs_[i] << ", ";
        }
        ss << dec<< endl;
        ss << "topKWorkerIds_      : " << topKWorkerIds_.size() << endl;
        for (size_t i = 0; i < topKWorkerIds_.size(); i ++)
        {
            ss << topKWorkerIds_[i] << ", ";
        }
        ss << endl;

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

        ss << "onto_rep_ : " <<endl;
        ss << onto_rep_.ToString();
        ss << "groupRep_ : " <<endl;
        ss << groupRep_.ToString();
        ss << "attrRep_ : " <<endl;
        ss << attrRep_.ToString();

        ss << "===================================" << endl;
        out << ss.str();
    }


    /// pre-fetched info for keyword search
    DistKeywordSearchInfo distSearchInfo_;

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

    /// for ec module.
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


    sf1r::faceted::OntologyRep onto_rep_;

    // a list, each element is a label tree for a property
    sf1r::faceted::GroupRep groupRep_;

    // a list, each element is a label array for an attribute name
    sf1r::faceted::OntologyRep attrRep_;

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

    MSGPACK_DEFINE(
        distSearchInfo_, rawQueryString_, encodingType_, collectionName_, analyzedQuery_,
        queryTermIdList_, totalCount_, topKDocs_, topKWorkerIds_, topKtids_, topKRankScoreList_,
        topKCustomRankScoreList_, propertyRange_, start_, count_, pageOffsetList_, propertyQueryTermList_,
        onto_rep_, groupRep_, attrRep_);
};

class DistSummaryMiningResult : public ErrorInfo
{
};
