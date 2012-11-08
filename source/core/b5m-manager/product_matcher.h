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
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/trie_policy.hpp>
#include <ext/pb_ds/tag_and_trait.hpp>

namespace sf1r {

    class ProductMatcher {


    public:
        typedef izenelib::util::UString UString;
        typedef idmlib::sim::StringSimilarity::Object SimObject;
        typedef uint32_t term_t;
        typedef std::vector<term_t> TermList;
        typedef std::vector<TermList> Suffixes;
        typedef boost::unordered_set<TermList> KeywordSet;
        struct CategoryNameApp
        {
            uint32_t cid;
            uint32_t depth;
        };
        struct AttributeApp
        {
            uint32_t spu_id;
            std::string attribute_name;
            bool is_optional;
        };
        struct SpuTitleApp
        {
            uint32_t spu_id;
            uint32_t pstart;
            uint32_t pend;
        };
        struct KeywordTag
        {
            std::vector<CategoryNameApp> category_name_apps;
            std::vector<AttributeApp> attribute_apps;
            std::vector<SpuTitleApp> spu_title_apps;
            KeywordTag& operator+=(const KeywordTag& another)
            {
                category_name_apps.insert(category_name_apps.end(), another.category_name_apps.begin(), another.category_name_apps.end());
                attribute_apps.insert(attribute_apps.end(), another.attribute_apps.begin(), another.attribute_apps.end());
                spu_title_apps.insert(spu_title_apps.end(), another.spu_title_apps.begin(), another.spu_title_apps.end());
                return *this;
            }
        };

        template<class T>
        struct vector_access_traits
        {
        public:
          typedef std::size_t size_type;
          typedef std::vector<T> key_type;
          typedef const key_type& key_const_reference;
          typedef T e_type;
          typedef typename key_type::const_iterator const_iterator;

          enum
            {
              max_size = 10000
            };

          // Returns a const_iterator to the firstelement of r_key.
          inline static const_iterator
          begin(key_const_reference r_key)
          { return r_key.begin(); }

          // Returns a const_iterator to the after-lastelement of r_key.
          inline static const_iterator
          end(key_const_reference r_key)
          { return r_key.end(); }

          // Maps an element to a position.
          inline static std::size_t
          e_pos(e_type e)
          {
              return (std::size_t)e;
          }
        };
        typedef __gnu_pbds::trie<std::vector<term_t>, KeywordTag, vector_access_traits<term_t> > TrieType;


        struct Category
        {
            std::string name;
            uint32_t cid;
            uint32_t parent_cid;
            bool is_parent;
            uint32_t depth;
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & name & cid & parent_cid & is_parent & depth;
            }
        };

        struct Attribute
        {
            std::string name;
            std::vector<std::string> values;
            bool is_optional;
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & name & values & is_optional;
            }
        };

        struct Product
        {
            std::string spid;
            std::string stitle;
            std::string scategory;
            double price; 
            std::vector<Attribute> attributes;
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & spid & stitle & scategory & price & attributes;
            }
        };
        typedef uint32_t PidType;
        typedef std::map<std::string, uint32_t> CategoryIndex;

        ProductMatcher(const std::string& path);
        ~ProductMatcher();
        bool IsOpen() const;
        bool Open();
        static void Clear(const std::string& path);
        bool Index(const std::string& scd_path);
        bool DoMatch(const std::string& scd_path);
        bool Process(Document& doc);

        void SetCmaPath(const std::string& path)
        { cma_path_ = path; }

    private:
        bool PriceMatch_(double p1, double p2);
        void Analyze_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);
        void AnalyzeChar_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);
        void AnalyzeCR_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);

        void AnalyzeImpl_(idmlib::util::IDMAnalyzer* analyzer, const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);


        void ParseAttributes_(const UString& ustr, std::vector<Attribute>& attributes);
        void GenSuffixes_(const std::vector<term_t>& term_list, Suffixes& suffixes);
        void GenSuffixes_(const std::string& text, Suffixes& suffixes);
        void ConstructSuffixTrie_(TrieType& trie);
        term_t GetTerm_(const UString& text);
        std::string GetText_(const TermList& tl);
        void GetTerms_(const std::string& text, std::vector<term_t>& term_list);
        void GetTerms_(const UString& text, std::vector<term_t>& term_list);
        void ConstructKeywords_(KeywordSet& keyword_set);
        void ConstructKeywordTrie_(const TrieType& suffix_trie, const KeywordSet& keyword_set, TrieType& trie);

        template<class T>
        static bool StartsWith_(const std::vector<T>& v1, const std::vector<T>& v2)
        {
            if(v2.size()>v1.size()) return false;
            for(uint32_t i=0;i<v2.size();i++)
            {
                if(v2[i]!=v1[i]) return false;
            }
            return true;
        }


    private:
        std::string path_;
        bool is_open_;
        std::string cma_path_;
        idmlib::sim::StringSimilarity string_similarity_;
        std::vector<Product> products_;
        term_t tid_;
        AttributeIdManager* aid_manager_;
        idmlib::util::IDMAnalyzer* analyzer_;
        idmlib::util::IDMAnalyzer* char_analyzer_;
        idmlib::util::IDMAnalyzer* chars_analyzer_;
        std::vector<std::string> keywords_thirdparty_;
        std::string test_docid_;
        std::string left_bracket_;
        std::string right_bracket_;
        std::vector<Category> category_list_;
        CategoryIndex category_index_;
        TrieType trie_;
        
    };
}

#endif

