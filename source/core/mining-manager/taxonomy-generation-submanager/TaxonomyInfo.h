///
/// @file TaxonomyInfo.h
/// @brief This class generate all labels in documents to support taxonomy information.
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2009
/// @date Updated 2009-11-21 01:01:45
///

#ifndef TAXONOMYINFO_H_
#define TAXONOMYINFO_H_

#include "TgTypes.h"
#include <mining-manager/MiningManagerDef.h>
#include <common/PropertyTermInfo.h>
#include <common/type_defs.h>
#include <configuration-manager/MiningConfig.h>
#include <idmlib/keyphrase-extraction/kpe_algorithm.h>
#include <idmlib/keyphrase-extraction/kpe_output.h>
#include <idmlib/util/file_object.h>

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>

#include <string>
#include <vector>

namespace idmlib
{
class NameEntityManager;
namespace nec
{
class NEC;
}
}

namespace sf1r
{

class LabelManager;
class TaxonomyInfo : public boost::noncopyable
{

    typedef std::pair<uint32_t, uint32_t> id2count_t;
    typedef std::pair<izenelib::util::UString, uint32_t> str2count_t;
    typedef idmlib::kpe::KPEOutput<true, true, true> OutputType ;
    typedef OutputType::function_type function_type ;
public:
    typedef idmlib::kpe::KPEAlgorithm<OutputType> kpe_type;

    TaxonomyInfo(const std::string& tgPath,
                 const TaxonomyPara& taxonomy_param,
                 boost::shared_ptr<LabelManager> labelManager,
                 idmlib::util::IDMAnalyzer* analyzer,
                 const std::string& sys_res_path);
    ~TaxonomyInfo();

public:

    bool Open();

    uint32_t GetMaxDocId()
    {
        return max_docid_;
    }

    void setKPECacheSize( uint32_t size)
    {
        kpeCacheSize_ = size;
    }

    /**
    * @brief Insert one document.
    * @param docid The document id.
    * @param termList The input term list.
    */
    void addDocument(uint32_t docid, const izenelib::util::UString& text);

    /**
    * @brief Delete one document.
    * @param docid The document id.
    * @return if is exist and deleted.
    */
    bool deleteDocument(uint32_t docid);

    /**
    * @brief Finish all update.
    */
    void finish();

    void ReleaseResource();


private:


    kpe_type* initKPE_();

    void callback_(
        const izenelib::util::UString& str
        , const std::vector<id2count_t>& id2countList
        , uint8_t score
        , const std::vector<id2count_t>& leftTermList
        , const std::vector<id2count_t>& rightTermList);
//     bool addDocument_(uint32_t docid, const std::vector<TgTerm>& termList, uint32_t& iBegin);

private:
    std::string path_;
    TaxonomyPara taxonomy_param_;
    idmlib::util::IDMAnalyzer* analyzer_;
    idmlib::util::IDMIdManager* id_manager_;
    boost::shared_ptr<LabelManager> labelManager_;
    std::string sys_res_path_;
    kpe_type* kpe_;
    function_type callback_func_;
    kpe_type::ScorerType* scorer_;
    static idmlib::NameEntityManager* nec_;
    bool enable_new_nec_;
    idmlib::nec::NEC* cn_nec_;
    uint32_t kpeCacheSize_;
    uint32_t max_docid_;
    uint32_t last_insert_docid_;
    uint32_t insert_count_;
    uint32_t label_processed_;
    idmlib::util::FileObject<uint32_t> max_docid_file_;

};


}
#endif
