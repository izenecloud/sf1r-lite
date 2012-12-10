#ifndef SF1R_MINING_SUFFIX_FMINDEXMANAGER_H_
#define SF1R_MINING_SUFFIX_FMINDEXMANAGER_H_

#include <common/type_defs.h>
#include <am/succinct/fm-index/fm_index.hpp>
#include <am/succinct/fm-index/fm_doc_array_manager.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>

namespace sf1r
{

class DocumentManager;
class FilterManager;

class FMIndexManager
{
public:
    typedef izenelib::am::succinct::fm_index::FMIndex<uint16_t> FMIndexType;
    typedef izenelib::am::succinct::fm_index::FMDocArrayMgr<uint16_t> FMDocArrayMgrType;
    typedef FMIndexType::MatchRangeT  RangeT;
    typedef FMIndexType::MatchRangeListT  RangeListT;
    typedef FMDocArrayMgrType::FilterRangeT FilterRangeT;

    enum PropertyFMType
    {
        LESS_DV,
        COMMON,
        LAST
    };

    FMIndexManager(const std::string& homePath,
            boost::shared_ptr<DocumentManager>& document_manager,
            boost::shared_ptr<FilterManager>& filter_manager);

    ~FMIndexManager();
    inline size_t docCount() const { return doc_count_; }
    void addProperties(const std::vector<std::string>& properties, PropertyFMType type);
    void getProperties(std::vector<std::string>& properties, PropertyFMType type) const;
    bool isStartFromLocalFM() const;
    void clearFMIData();
    void useOldDocCount(const FMIndexManager* old_fmi_manager);
    bool buildCommonProperties(const FMIndexManager* old_fmi_manager);
    void swapCommonPropertiesData(FMIndexManager* old_fmi_manager);
    void buildLessDVProperties();
    void buildExternalFilter();

    void setFilterList(std::vector<std::vector<FMDocArrayMgrType::FilterItemT> > &filter_list);
    bool getFilterRange(size_t filter_index, const RangeT &filter_id_range, RangeT &match_range) const;

    void getMatchedDocIdList(
        const std::string& property,
        const RangeListT& match_ranges, size_t max_docs,
        std::vector<uint32_t>& docid_list, std::vector<size_t>& doclen_list) const;

    void convertMatchRanges(
        const std::string& property,
        size_t max_docs,
        RangeListT& match_ranges,
        std::vector<double>& max_match_list) const;

    size_t longestSuffixMatch(
        const std::string& property,
        const izenelib::util::UString& pattern,
        RangeListT& match_ranges) const;

    size_t backwardSearch(const std::string& prop, const izenelib::util::UString& pattern, RangeT& match_range) const;

    void getTopKDocIdListByFilter(
            const std::string& property,
            const std::vector<size_t> &prop_id_list,
            const std::vector<RangeListT> &filter_ranges,
            const RangeListT &match_ranges_list,
            const std::vector<double> &max_match_list,
            size_t max_docs,
            std::vector<std::pair<double, uint32_t> > &res_list) const;

    void getDocLenList(const std::vector<uint32_t>& docid_list, std::vector<size_t>& doclen_list) const;
    void getLessDVStrLenList(const std::string& property, const std::vector<uint32_t>& dvid_list, std::vector<size_t>& dvlen_list) const;

    void saveAll();
    bool loadAll();

private:
    bool initAndLoadOldDocs(const FMIndexManager* old_fmi_manager);
    void appendDocs(size_t last_docid);

    size_t putFMIndexToDocArrayMgr(FMIndexType* fmi);

    void reconstructText(const std::string& prop_name,
        const std::vector<uint32_t>& del_docid_list,
        std::vector<uint16_t>& orig_text) const;

    struct PropertyFMIndex
    {
        PropertyFMIndex()
            :type(COMMON), docarray_mgr_index((size_t)-1)
        {
        }
        PropertyFMType type;
        size_t  docarray_mgr_index;
        boost::shared_ptr<FMIndexType> fmi;
    };

    std::string data_root_path_;
    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<FilterManager> filter_manager_;
    size_t doc_count_;

    std::map<std::string, PropertyFMIndex> all_fmi_;
    typedef std::map<std::string, PropertyFMIndex>::iterator FMIndexIter;
    typedef std::map<std::string, PropertyFMIndex>::const_iterator FMIndexConstIter;
    FMDocArrayMgrType  docarray_mgr_;
};

}

#endif
