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

    bool isOptionGatherInfo() const
    {
        return effective_ && option_ == OPTION_GATHER_INFO;
    }

    bool isOptionCarriedInfo() const
    {
        return effective_ && option_ == OPTION_CARRIED_INFO;
    }

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

class DistSummaryMiningResult : public ErrorInfo
{
};
