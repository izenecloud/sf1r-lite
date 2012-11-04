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
#include <idmlib/util/idm_id_manager.h>
#include <document-manager/Document.h>
#include <idmlib/keyphrase-extraction/kpe_task.h>
#include <idmlib/similarity/string_similarity.h>

namespace sf1r {

    class ProductMatcher {


    public:
        typedef izenelib::util::UString UString;
        typedef boost::unordered_map<std::string, std::string> CategoryGroup;
        typedef boost::unordered_map<std::string, uint32_t> CategoryTermCount;
        typedef uint64_t hash_t;
        typedef std::pair<uint32_t, uint32_t> id2count_t;
        typedef std::vector<uint32_t> idvec_t;
        typedef std::vector<id2count_t> id2countvec_t;
        typedef idmlib::sim::StringSimilarity::Object SimObject;
        typedef std::vector<std::string> EditCategoryKeywords;
        typedef izenelib::am::leveldb::Table<std::string, UString> CRResult;

        struct CategoryProb
        {
            std::string scategory;
            double score;
        };

        struct CategoryKeyword
        {
            std::string keyword;
            std::vector<CategoryProb> probs;
        };

        struct SpuCategory
        {
            std::string scategory;
            idmlib::sim::StringSimilarity::Object obj;
            uint32_t cid;
            uint32_t parent_cid;
            bool is_parent;
        };

        //typedef boost::unordered_map<std::string, double> CategorySS;
        typedef boost::unordered_map<std::string, std::vector<uint32_t> > CategorySpuIndex;

        typedef boost::unordered_map<std::string, uint32_t> CategoryIndex;

        struct TitleCategoryScore
        {
            TitleCategoryScore()
            : position(-1.0), category_sim(-1.0), spu_sim(-1.0)
            {
            }
            //std::string scategory;
            uint32_t category_index;
            //std::vector<double> position;
            double position;
            double category_sim;
            double spu_sim;
        };
        typedef std::pair<uint32_t, double> CategoryScore; //category_index, score;
        typedef std::vector<CategoryScore> CategoryScoreList;
        //typedef trie<std::string, CategoryScoreList> TrieType;
        typedef boost::unordered_map<std::string, CategoryScoreList> TrieType;

        struct Product
        {
            std::string spid;
            std::string stitle;
            std::string scategory;
            double price; friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & spid & stitle & scategory & price;
            }
        };
        typedef uint32_t PidType;
        typedef std::pair<PidType, double> PidPair;
        typedef std::vector<PidPair> PidList;
        typedef uint64_t CAttribId;
        typedef std::map<AttribId, PidList> A2PMap;
        typedef std::set<uint32_t> CidSet;
        ProductMatcher(const std::string& path);
        ~ProductMatcher();
        bool IsOpen() const;
        bool Open();
        static void Clear(const std::string& path);
        void LoadCategoryGroup(const std::string& file);
        void InsertCategoryGroup(const std::vector<std::string>& group);
        bool Index(const std::string& scd_path);
        bool GetMatched(const Document& doc, uint32_t count, std::vector<Product>& products);
        bool GetMatched(const Document& doc, Product& product);
        bool GetCategory(const Document& doc, UString& category);
        bool GetProductInfo(const PidType& pid, Product& product);
        bool DoMatch(const std::string& scd_path);
        bool IndexCR();
        bool DoCR(const std::string& scd_path); //do category recognizer

        void SetCmaPath(const std::string& path)
        { cma_path_ = path; }

    private:
        bool PriceMatch_(double p1, double p2);
        uint32_t GetCategoryId_(const UString& category);
        AttribId GenAID_(const UString& category, const std::vector<UString>& value_list, AttribId& aid);
        CAttribId GetCAID_(AttribId aid, const UString& category);
        CAttribId GetCAID_(AttribId aid);
        void GetAttribIdSet(const UString& category, const izenelib::util::UString& value, std::set<AttribId>& aid_set);
        void NormalizeText_(const izenelib::util::UString& text, izenelib::util::UString& ntext);
        void Analyze_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);
        void AnalyzeChar_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);
        void AnalyzeCR_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);

        void AnalyzeImpl_(idmlib::util::IDMAnalyzer* analyzer, const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);

        UString GetAttribRep_(const UString& category, const UString& text);
        void WriteCategoryGroup_();
        void NgramProcess_(const std::vector<idmlib::kpe::NgramInCollection>& data);
        std::string GetNgramString_(const std::vector<uint32_t>& id_list);
        void OutputNgram_(const idmlib::kpe::NgramInCollection& ngram);
        //void GetCategorySS_(UString title, CategorySS& ss);
        void GetCategorySpuIndexes_(const std::string& scategory, std::vector<uint32_t>& indexes);
        double ComputeSpuSim_(const SimObject& obj, const std::string& scategory);
        void GenerateCategoryTokens_(const std::vector<SpuCategory>& spu_categories);
        void LoadCategoryKeywords_(const std::string& file);
        void LoadCategories_(const std::string& file);
        void CRInit_();


    private:
        std::string path_;
        bool is_open_;
        std::string cma_path_;
        idmlib::sim::StringSimilarity string_similarity_;
        CategoryGroup category_group_;
        CidSet cid_set_;
        AttributeIdManager* aid_manager_;
        A2PMap a2p_;
        std::vector<Product> products_;
        std::ofstream logger_;
        idmlib::util::IDMAnalyzer* analyzer_;
        idmlib::util::IDMAnalyzer* char_analyzer_;
        idmlib::util::IDMAnalyzer* chars_analyzer_;
        std::string test_docid_;
        idmlib::util::IDMIdManager* id_manager_;
        CategoryTermCount category_term_count_;
        std::ofstream cr_ofs_;
        std::vector<idmlib::sim::StringSimilarity::Object> ss_objects_;
        EditCategoryKeywords edit_category_keywords_;
        CategorySpuIndex category_spu_index_;
        std::string left_bracket_;
        std::string right_bracket_;
        std::vector<SpuCategory> category_list_;
        CategoryIndex category_index_;
        TrieType trie_;
        CRResult* cr_result_;
        
    };
}

#endif

