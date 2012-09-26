#ifndef SF1R_B5MMANAGER_PRODUCTMATCHER_H_
#define SF1R_B5MMANAGER_PRODUCTMATCHER_H_
#include "ngram_synonym.h"
#include "b5m_helper.h"
#include "b5m_types.h"
#include "attribute_id_manager.h"
#include <boost/regex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/serialization/set.hpp>
#include <ir/id_manager/IDManager.h>
#include <am/sequence_file/ssfr.h>
#include <am/leveldb/Table.h>
#include <idmlib/util/idm_analyzer.h>
#include <document-manager/Document.h>

namespace sf1r {

    class ProductMatcher {


    public:
        typedef izenelib::util::UString UString;
        struct Product
        {
            std::string spid;
            std::string stitle;
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & spid & stitle;
            }
        };
        typedef uint32_t PidType;
        typedef std::pair<PidType, double> PidPair;
        typedef std::vector<PidPair> PidList;
        typedef uint64_t CAttribId;
        typedef std::map<CAttribId, PidList> A2PMap;
        typedef std::set<uint32_t> CidSet;
        ProductMatcher(const std::string& path);
        ~ProductMatcher();
        bool IsOpen() const;
        bool Open();
        static void Clear(const std::string& path);
        bool Index(const std::string& scd_path);
        bool GetMatched(const Document& doc, Product& product);
        bool GetProductInfo(const PidType& pid, Product& product);
        bool DoMatch(const std::string& scd_path);

        void SetCmaPath(const std::string& path)
        { cma_path_ = path; }

    private:
        uint32_t GetCategoryId_(const UString& category);
        CAttribId GetCAID_(AttribId aid, const UString& category);
        CAttribId GetCAID_(AttribId aid);
        void GetAttribIdSet(const izenelib::util::UString& value, std::set<AttribId>& aid_set);
        void NormalizeText_(const izenelib::util::UString& text, izenelib::util::UString& ntext);
        void Analyze_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);
        void AnalyzeChar_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);

        void AnalyzeImpl_(idmlib::util::IDMAnalyzer* analyzer, const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);


    private:
        std::string path_;
        bool is_open_;
        std::string cma_path_;
        CidSet cid_set_;
        AttributeIdManager* aid_manager_;
        A2PMap a2p_;
        std::vector<Product> products_;
        std::ofstream logger_;
        idmlib::util::IDMAnalyzer* analyzer_;
        idmlib::util::IDMAnalyzer* char_analyzer_;
    };
}

#endif

