#ifndef SF1R_MINING_SUFFIX_MATCH_SPU_PRODUCT_CLASSIFER_HPP_
#define SF1R_MINING_SUFFIX_MATCH_SPU_PRODUCT_CLASSIFER_HPP_

#include <document-manager/Document.h>

#include <am/succinct/fm-index/fm_index.hpp>
#include <am/succinct/fm-index/fm_doc_array_manager.hpp>
#include <util/singleton.h>
#include <util/ustring/UString.h>
#include <cache/IzeneCache.h>

#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>
#include <map>

namespace cma
{
class Analyzer;
class Knowledge;
}

namespace sf1r
{
class DocContainer;
class TuanEnricher;
using izenelib::util::UString;
class SPUProductClassifier
{
public:
    typedef izenelib::am::succinct::fm_index::FMIndex<uint16_t> FMIndexType;
    typedef izenelib::am::succinct::fm_index::FMDocArrayMgr<uint16_t> FMDocArrayMgrType;
    typedef FMIndexType::MatchRangeT RangeT;
    typedef FMIndexType::MatchRangeListT RangeListT;

    SPUProductClassifier();

    ~SPUProductClassifier();

    static SPUProductClassifier* Get()
    {
        return ::izenelib::util::Singleton<SPUProductClassifier>::get();
    }

    void Open(const std::string& resource);

    bool GetProductCategory(
        const std::string& query,
        std::vector<UString>& frontCategories);

    bool GetEnrichedQuery(
        const std::string& query,
        std::string& enriched);

private:
    void InitOnce_(const std::string& resource);

    std::string resource_path_;
    cma::Analyzer* analyzer_;
    cma::Knowledge* knowledge_;
    DocContainer* doc_container_;
    TuanEnricher* tuan_enricher_;
    izenelib::cache::IzeneCache<uint32_t, Document, izenelib::util::ReadWriteLock> documentCache_;

    struct PropertyFMIndex
    {
        PropertyFMIndex()
            : docarray_mgr_index((size_t)-1)
        {
        }

        size_t docarray_mgr_index;
        boost::shared_ptr<FMIndexType> fmi;
    };

    std::string data_root_path_;
    std::vector<std::string> properties_;
    size_t doc_count_;
    std::map<std::string, PropertyFMIndex> all_fmi_;
    typedef std::map<std::string, PropertyFMIndex>::iterator FMIndexIter;
    typedef std::map<std::string, PropertyFMIndex>::const_iterator FMIndexConstIter;
    FMDocArrayMgrType docarray_mgr_;

};

}

#endif

