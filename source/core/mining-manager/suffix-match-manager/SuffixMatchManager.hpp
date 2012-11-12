#ifndef SF1R_MINING_SUFFIX_MATCHMANAGER_H_
#define SF1R_MINING_SUFFIX_MATCHMANAGER_H_

#include <common/type_defs.h>
#include <am/succinct/fm-index/fm_index.hpp>
#include <query-manager/ActionItem.h>
#include <mining-manager/group-manager/GroupParam.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>

namespace cma
{
class Analyzer;
class Knowledge;
}

namespace sf1r
{

class DocumentManager;
class FilterManager;
namespace faceted
{
    class GroupManager;
    class AttrManager;
}
class NumericPropertyTableBuilder;

class SuffixMatchManager
{
    typedef izenelib::am::succinct::fm_index::FMIndex<uint16_t> FMIndexType;

public:
    SuffixMatchManager(
            const std::string& homePath,
            const std::string& property,
            const std::string& dicpath,
            boost::shared_ptr<DocumentManager>& document_manager,
            faceted::GroupManager* groupmanager,
            faceted::AttrManager* attrmanager,
            NumericPropertyTableBuilder* numeric_tablebuilder);

    ~SuffixMatchManager();
    void setGroupFilterProperty(std::vector<std::string>& propertys);
    void setAttrFilterProperty(std::vector<std::string>& propertys);
    void setNumberFilterProperty(std::vector<std::string>& propertys,
        std::vector<int32_t>& amplification);

    void buildCollection();
    void buildTokenizeDic();

    size_t longestSuffixMatch(
            const izenelib::util::UString& pattern,
            size_t max_docs,
            std::vector<uint32_t>& docid_list,
            std::vector<float>& score_list) const;

    size_t AllPossibleSuffixMatch(
            const izenelib::util::UString& pattern,
            size_t max_docs,
            std::vector<uint32_t>& docid_list,
            std::vector<float>& score_list,
            const SearchingMode::SuffixMatchFilterMode& filter_mode,
            const std::vector<QueryFiltering::FilteringType>& filter_param,
            const faceted::GroupParam& group_param) const;


private:
    bool getAllFilterRangeFromGroupLable(
            const faceted::GroupParam& group_param,
            std::vector<FMIndexType::FilterRangeT>& filter_range_list) const;
    bool getAllFilterRangeFromAttrLable(
            const faceted::GroupParam& group_param,
            std::vector<FMIndexType::FilterRangeT>& filter_range_list) const;
    bool getAllFilterRangeFromFilterParam(
            const std::vector<QueryFiltering::FilteringType>& filter_param,
            std::vector<FMIndexType::FilterRangeT>& filter_range_list) const;

    std::string data_root_path_;
    std::string fm_index_path_;
    std::string orig_text_path_;

    std::string property_;
    std::vector<std::string> group_property_list_;
    std::vector<std::string> attr_property_list_;
    std::set<std::string> number_property_list_;
    std::string tokenize_dicpath_;
    boost::shared_ptr<DocumentManager> document_manager_;
    size_t last_doc_id_;

    cma::Analyzer* analyzer_;
    cma::Knowledge* knowledge_;

    boost::shared_ptr<FMIndexType> fmi_;

    typedef boost::shared_mutex MutexType;
    typedef boost::shared_lock<MutexType> ReadLock;
    typedef boost::unique_lock<MutexType> WriteLock;

    mutable MutexType mutex_;
    boost::shared_ptr<FilterManager> filter_manager_;
};

}

#endif
