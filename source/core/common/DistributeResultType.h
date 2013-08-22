/**
 * @file DistributeResultType.h
 * @author Zhongxia Li
 * @date Aug 12, 2011
 * @brief Distributed search result
 */
#include <vector>
#include <stdint.h>

class DistKeywordSearchInfo
{
public:

    DistKeywordSearchInfo()
        : isDistributed_(false)
        , effective_(false)
        , include_summary_data_(false)
        , option_(OPTION_NONE)
        , nodeType_(NODE_MASTER)
    {
    }

    static const int8_t OPTION_NONE = 0x0;
    static const int8_t OPTION_GATHER_INFO  = 0x1;
    static const int8_t OPTION_CARRIED_INFO = 0x2;

    static const int8_t NODE_MASTER = 0x0;
    static const int8_t NODE_WORKER = 0x1;

    bool isDistributed_;
    /// @brief whether distributed search is effective
    bool effective_;
    bool include_summary_data_;

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
    std::vector<std::pair<std::string, std::vector<std::string> > > sortPropertyStrDataList_;

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
        isDistributed_ = other.isDistributed_;
        swap(effective_, other.effective_);
        swap(include_summary_data_, other.include_summary_data_);
        swap(option_, other.option_);
        swap(nodeType_, other.nodeType_);
        dfmap_.swap(other.dfmap_);
        ctfmap_.swap(other.ctfmap_);
        maxtfmap_.swap(other.maxtfmap_);
        sortPropertyList_.swap(other.sortPropertyList_);
        sortPropertyInt32DataList_.swap(other.sortPropertyInt32DataList_);
        sortPropertyInt64DataList_.swap(other.sortPropertyInt64DataList_);
        sortPropertyFloatDataList_.swap(other.sortPropertyFloatDataList_);
        sortPropertyStrDataList_.swap(other.sortPropertyStrDataList_);
    }

    MSGPACK_DEFINE(isDistributed_, effective_, include_summary_data_, option_, nodeType_, dfmap_, ctfmap_, maxtfmap_, sortPropertyList_,
        sortPropertyInt32DataList_, sortPropertyInt64DataList_, sortPropertyFloatDataList_, sortPropertyStrDataList_);
};

class DistSummaryMiningResult : public ErrorInfo
{
};
