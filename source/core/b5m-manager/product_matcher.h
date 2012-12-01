#ifndef SF1R_B5MMANAGER_PRODUCTMATCHER_H_
#define SF1R_B5MMANAGER_PRODUCTMATCHER_H_
#include "brand_db.h"
#include "ngram_synonym.h"
#include "b5m_helper.h"
#include "b5m_types.h"
#include "attribute_id_manager.h"
#include <boost/regex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/serialization/set.hpp>
#include <boost/operators.hpp>
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
        typedef uint32_t position_t;
        typedef std::pair<position_t, position_t>  Position;
        
        struct WeightType
        {
            WeightType()
            :cweight(0.0), aweight(0.0), tweight(0.0), kweight(1.0)
             , paweight(0.0), paratio(0.0), type_match(false)
            {
            }
            double cweight;
            double aweight;
            double tweight;
            double kweight;
            double paweight;
            double paratio;
            bool type_match;
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & cweight & aweight & tweight & kweight & paweight;
            }

            inline double sum() const
            {
                return (cweight+aweight+tweight)*kweight;
            }

            bool operator<(const WeightType& another) const
            {
                return sum()<another.sum();
            }
        };

        struct ResultVectorItem
        {
            uint32_t cid;
            uint32_t spu_id;
            double score;
            WeightType weight;
            //double paweight;
            bool is_given_category;
            bool operator<(const ResultVectorItem& another) const
            {
                if(score==another.score)
                {
                    return is_given_category;
                }
                return score>another.score;
            }
        };

        template<class K, class V>
        class dmap : public boost::unordered_map<K,V>
        {
            //typedef std::map<K,V> BaseType;
            typedef boost::unordered_map<K,V> BaseType;
        public:
            dmap():BaseType()
            {
            }
            dmap(const V& default_value):d_(default_value)
            {
            }

            V& operator[] (const K& k)
            {
                return (*((this->insert(make_pair(k,d_))).first)).second;
            }


        private:
            V d_;
            
        };
        template<class K, class V>
        class stdmap : public std::map<K,V>
        {
            typedef std::map<K,V> BaseType;
        public:
            stdmap():BaseType()
            {
            }
            stdmap(const V& default_value):d_(default_value)
            {
            }

            V& operator[] (const K& k)
            {
                return (*((this->insert(make_pair(k,d_))).first)).second;
            }

            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & *(dynamic_cast<BaseType*>(this));
            }

        private:
            V d_;
            
        };

        typedef dmap<std::string, double> StringToScore;
        struct CategoryNameApp 
          : boost::less_than_comparable<CategoryNameApp> 
           //,boost::equality_comparable<CategoryNameApp>
        {
            uint32_t cid;
            uint32_t depth;
            bool is_complete;
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & cid & depth & is_complete;
            }

            bool operator<(const CategoryNameApp& another) const
            {
                return cid<another.cid;
                //if(cid<another.cid) return true;
                //else if(cid>another.cid) return false;
                //else if(depth<another.depth) return true;
                //else if(depth>another.depth) return false;
                //return false;
            }

            //bool operator==(const CategoryNameApp& another) const
            //{
                //return cid==another.cid&&depth==another.depth;
            //}
        };
        struct AttributeApp
          : boost::less_than_comparable<AttributeApp> ,boost::equality_comparable<AttributeApp>
        {
            uint32_t spu_id;
            std::string attribute_name;
            bool is_optional;
            //bool is_complete;
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & spu_id & attribute_name & is_optional;
            }
            bool operator<(const AttributeApp& another) const
            {
                if(spu_id<another.spu_id) return true;
                else if(spu_id>another.spu_id) return false;
                return attribute_name<another.attribute_name;
            }

            bool operator==(const AttributeApp& another) const
            {
                return spu_id==another.spu_id&&attribute_name==another.attribute_name;
                //return spu_id==another.spu_id;
            }
        };
        struct SpuTitleApp
          : boost::less_than_comparable<SpuTitleApp> ,boost::equality_comparable<SpuTitleApp>
        {
            uint32_t spu_id;
            uint32_t pstart;
            uint32_t pend;
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & spu_id & pstart & pend;
            }
            bool operator<(const SpuTitleApp& another) const
            {
                if(spu_id<another.spu_id) return true;
                else if(spu_id>another.spu_id) return false;
                else
                {
                    return pstart<another.pstart;
                }
            }

            bool operator==(const SpuTitleApp& another) const
            {
                return spu_id==another.spu_id&&pstart==another.pstart;
            }
        };
        typedef stdmap<std::string, uint32_t> KeywordTypeApp;
        struct KeywordTag
        {
            KeywordTag();
            TermList term_list;
            std::vector<CategoryNameApp> category_name_apps;
            std::vector<AttributeApp> attribute_apps;
            std::vector<SpuTitleApp> spu_title_apps;
            KeywordTypeApp type_app;
            //double cweight; //not serialized, runtime property
            //double aweight;
            double kweight;
            uint8_t ngram;
            std::vector<Position> positions;

            template<class T>
            void SortAndUnique(std::vector<T>& vec)
            {
                std::sort(vec.begin(), vec.end());
                vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
            }
            void Flush();
            void Append(const KeywordTag& another, bool is_complete);
            bool Combine(const KeywordTag& another);
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & term_list & category_name_apps & attribute_apps & spu_title_apps & type_app;
            }
        };

        //typedef std::map<TermList, KeywordTag> KeywordMap;
        //typedef std::pair<TermList, KeywordTag> KeywordItem;
        typedef std::vector<KeywordTag> KeywordVector;
        //struct KeywordApp
        //{
            //KeywordItem item;
            //KeywordTypeApp type_app;
        //};
        //typedef boost::unordered_map<Position, KeywordApp> KeywordAppMap;

        struct ProductCandidate
        {
            ProductCandidate()
            : attribute_score(0.0)
            {
            }
            dmap<std::string, double> attribute_score;
            double GetAWeight() const
            {
                double r = 0.0;
                for(dmap<std::string, double>::const_iterator it = attribute_score.begin();it!=attribute_score.end();++it)
                {
                    r+=it->second;
                }
                return r;
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
        //typedef __gnu_pbds::trie<std::vector<term_t>, KeywordTag, vector_access_traits<term_t> > TrieType;
        typedef std::map<std::vector<term_t>, KeywordTag> TrieType;


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
            std::string GetValue() const
            {
                std::string result;
                for(uint32_t i=0;i<values.size();i++)
                {
                    if(!result.empty()) result+="/";
                    result+=values[i];
                }
                return result;
            }
        };

        struct Product
        {
            std::string spid;
            std::string stitle;
            std::string scategory;
            double price; 
            std::vector<Attribute> attributes;
            //WeightType weight;
            double aweight;
            double tweight;
            SimObject title_obj;
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & spid & stitle & scategory & price & attributes & aweight & tweight & title_obj;
            }
        };
        typedef uint32_t PidType;
        typedef std::map<std::string, uint32_t> CategoryIndex;
        typedef std::map<std::string, uint32_t> ProductIndex;
        typedef std::vector<uint32_t> IdList;
        typedef std::map<uint32_t, IdList> IdToIdList;
        typedef boost::unordered_map<uint32_t, UString> IdManager;

        ProductMatcher(const std::string& path);
        ~ProductMatcher();
        bool IsOpen() const;
        bool Open();
        static void Clear(const std::string& path, int mode=3);
        bool Index(const std::string& scd_path);
        void Test(const std::string& scd_path);
        bool DoMatch(const std::string& scd_path);
        bool Process(const Document& doc, Product& result_product);
        bool ProcessBook(const Document& doc, Product& result_product);
        bool GetProduct(const std::string& pid, Product& product);

        void SetCmaPath(const std::string& path)
        { cma_path_ = path; }

        void SetUsePriceSim(bool sim)
        { use_price_sim_ = sim; }

    private:
        bool PriceMatch_(double p1, double p2);
        double PriceSim_(double offerp, double spup);
        void Analyze_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);
        void AnalyzeChar_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);
        void AnalyzeCR_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);

        void AnalyzeImpl_(idmlib::util::IDMAnalyzer* analyzer, const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);



        void ParseAttributes_(const UString& ustr, std::vector<Attribute>& attributes);
        void GenSuffixes_(const std::vector<term_t>& term_list, Suffixes& suffixes);
        void GenSuffixes_(const std::string& text, Suffixes& suffixes);
        void ConstructSuffixTrie_(TrieType& trie);
        term_t GetTerm_(const UString& text);
        term_t GetTerm_(const std::string& text);
        std::string GetText_(const TermList& tl);
        void GetTerms_(const std::string& text, std::vector<term_t>& term_list);
        void GetTerms_(const UString& text, std::vector<term_t>& term_list);
        void GetCRTerms_(const UString& text, std::vector<term_t>& term_list);
        void ConstructKeywords_();
        void AddKeyword_(const UString& text);
        void ConstructKeywordTrie_(const TrieType& suffix_trie);
        void GetKeywordVector_(const TermList& term_list, KeywordVector& keyword_vector);
        void Compute_(const Document& doc, const TermList& term_list, KeywordVector& keyword_vector, uint32_t& cid, uint32_t& pid);
        uint32_t GetCidBySpuId_(uint32_t spu_id);

        bool SpuMatched_(const WeightType& weight, const Product& p) const;
        int SelectKeyword_(const KeywordTag& tag1, const KeywordTag& tag2) const;

        template<class K, class V>
        void GetSortedVector_(const std::map<K, V>& map, std::vector<std::pair<V, K> >& vec, uint32_t max=0)
        {
            for(typename std::map<K,V>::const_iterator it = map.begin();it!=map.end();it++)
            {
                vec.push_back(std::make_pair(it->second, it->first));
            }
            std::sort(vec.begin(), vec.end(), std::greater<std::pair<V, K> >());
            if(max>0&&vec.size()>max)
            {
                vec.resize(max);
            }
        }

        template<class T>
        void SortVectorDesc_(std::vector<T>& vec)
        {
            std::sort(vec.begin(), vec.end(), std::greater<T>());
        }

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
        bool use_price_sim_;
        idmlib::sim::StringSimilarity string_similarity_;
        std::vector<Product> products_;
        term_t tid_;
        AttributeIdManager* aid_manager_;
        IdManager id_manager_;
        idmlib::util::IDMAnalyzer* analyzer_;
        idmlib::util::IDMAnalyzer* char_analyzer_;
        idmlib::util::IDMAnalyzer* chars_analyzer_;
        std::vector<std::string> keywords_thirdparty_;
        KeywordSet not_keywords_;
        std::string test_docid_;
        std::string left_bracket_;
        std::string right_bracket_;
        term_t left_bracket_term_;
        term_t right_bracket_term_;
        std::vector<Category> category_list_;
        IdToIdList cid_to_pids_;
        CategoryIndex category_index_;
        ProductIndex product_index_;
        KeywordSet keyword_set_;
        TrieType trie_;
        KeywordVector keyword_vector_;

        const static double optional_weight_ = 0.2;
        
    };
}

#endif

