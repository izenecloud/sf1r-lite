#include <common/type_defs.h>
#include <am/succinct/fm-index/fm_index.hpp>

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

class SuffixMatchManager
{
    typedef izenelib::am::succinct::fm_index::FMIndex<uint16_t> FMIndexType;

public:
    SuffixMatchManager(
            const std::string& homePath,
            const std::string& property,
            const std::string& dicpath,
            boost::shared_ptr<DocumentManager>& document_manager);

    ~SuffixMatchManager();
    void setGroupFilterProperty(std::vector<std::string>& propertys);

    void buildCollection();
    void buildTokenizeDic();

    size_t longestSuffixMatch(const izenelib::util::UString& pattern, size_t max_docs, std::vector<uint32_t>& docid_list, std::vector<float>& score_list) const;
    size_t AllPossibleSuffixMatch(const izenelib::util::UString& pattern,
        size_t max_docs, std::vector<uint32_t>& docid_list,
        std::vector<float>& score_list, const izenelib::util::UString& filter_str = izenelib::util::UString()) const;


private:
    void loadFilterInvertedData(std::vector< FMIndexType::FilterItemT >& filter_inverted_data);
    void saveFilterInvertedData(const std::vector< FMIndexType::FilterItemT >& filter_inverted_data) const;

    std::string data_root_path_;
    std::string fm_index_path_;
    std::string property_;
    std::vector<std::string>  group_property_list_;
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
};

}
