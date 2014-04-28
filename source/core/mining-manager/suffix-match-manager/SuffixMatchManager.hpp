#ifndef SF1R_MINING_SUFFIX_MATCHMANAGER_H_
#define SF1R_MINING_SUFFIX_MATCHMANAGER_H_

#include "SuffixMatchMiningTask.hpp"
#include <common/type_defs.h>
#include <am/succinct/fm-index/fm_index.hpp>
#include <query-manager/ActionItem.h>
#include <mining-manager/group-manager/GroupParam.h>
#include <common/ResultType.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <list>

namespace sf1r
{
using izenelib::util::UString;
class DocumentManager;
class FilterManager;
class FMIndexManager;
namespace b5m {
class ProductMatcher;
}
class CategoryClassifyTable;
class FuzzyNormalizer;

namespace faceted
{
    class GroupManager;
    class AttrManager;
}
class NumericPropertyTableBuilder;

class SuffixMatchManager
{
public:
    SuffixMatchManager(
            const std::string& homePath,
            boost::shared_ptr<DocumentManager>& document_manager,
            faceted::GroupManager* groupmanager,
            faceted::AttrManager* attrmanager,
            NumericPropertyTableBuilder* numeric_tablebuilder,
            FuzzyNormalizer* fuzzyNormalizer);

    ~SuffixMatchManager();

    void addFMIndexProperties(const std::vector<std::string>& property_list, const VirtualConfig& virtualProperty, int type, bool finished = false);

    bool isStartFromLocalFM() const;

    size_t longestSuffixMatch(
            const std::string& pattern,
            std::vector<std::string> search_in_properties,
            size_t max_docs,
            std::vector<std::pair<double, uint32_t> >& res_list) const;

    size_t AllPossibleSuffixMatch(
            bool use_synonym,            
            const std::list<std::pair<UString, double> >& major_tokens,
            const std::list<std::pair<UString, double> >& minor_tokens,
            const std::vector<std::string>& search_in_properties,
            size_t max_docs,
            const SearchingMode::SuffixMatchFilterMode& filter_mode,
            const std::vector<QueryFiltering::FilteringType>& filter_param,
            const faceted::GroupParam& group_param,
            std::vector<std::pair<double, uint32_t> >& res_list,
            double rank_boundary);

    SuffixMatchMiningTask* getMiningTask();
    
    bool buildMiningTask();

    boost::shared_ptr<FilterManager>& getFilterManager();

    void updateFmindex();

private:
    typedef izenelib::am::succinct::fm_index::FMIndex<uint16_t> FMIndexType;
    typedef FMIndexType::MatchRangeListT RangeListT;
    bool GetSynonymSet_(const UString& pattern, std::vector<UString>& synonym_set, int& setid);
    bool GetSynonymId_(const UString& pattern, int& setid);
    void ExpandSynonym_(const std::vector<std::pair<UString, double> >& tokens, std::vector<std::vector<std::pair<UString, double> > >& refine_tokens, size_t& major_size);
    bool getAllFilterRangeFromGroupLable_(
            const faceted::GroupParam& group_param,
            std::vector<size_t>& prop_id_list,
            std::vector<RangeListT>& filter_range_list) const;
    bool getAllFilterRangeFromAttrLable_(
            const faceted::GroupParam& group_param,
            std::vector<size_t>& prop_id_list,
            std::vector<RangeListT>& filter_range_list) const;
    bool getAllFilterRangeFromFilterParam_(
            const std::vector<QueryFiltering::FilteringType>& filter_param,
            std::vector<size_t>& prop_id_list,
            std::vector<RangeListT>& filter_range_list) const;

    std::string data_root_path_;

    boost::shared_ptr<DocumentManager> document_manager_;
    size_t last_doc_id_;

    boost::shared_ptr<FMIndexManager> fmi_manager_;
    boost::shared_ptr<FilterManager> filter_manager_;
    boost::shared_ptr<FilterManager> new_filter_manager;

    SuffixMatchMiningTask* suffixMatchTask_;

    typedef boost::shared_mutex MutexType;
    typedef boost::shared_lock<MutexType> ReadLock;
    typedef boost::unique_lock<MutexType> WriteLock;

    mutable MutexType mutex_;

    FuzzyNormalizer* fuzzyNormalizer_;
};

}

#endif
