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
        //struct Double : boost::less_than_comparable<Double>
                        //,boost::equality_comparable<Double>
                        //,boost::addable<Double>
                        //,boost::subtractable<Double>
                        //,boost::multipliable<Double>
                        //,boost::dividable<Double>
        //{
            //double value;

            //Double():value(0.0)
            //{
            //}
            //Double(double d):value(d)
            //{
            //}

            //bool operator<(const Double& another) const
            //{
                //return value<another.value;
            //}

            //bool operator==(const Double& another) const
            //{
                //return value==another.value;
            //}

            //Double& operator+=(const Double& another)
            //{
                //value+=another.value;
                //return *this;
            //}
            //Double& operator-=(const Double& another)
            //{
                //value-=another.value;
                //return *this;
            //}
            //Double& operator*=(const Double& another)
            //{
                //value*=another.value;
                //return *this;
            //}
            //Double& operator/=(const Double& another)
            //{
                //value/=another.value;
                //return *this;
            //}

            //operator double () const
            //{
                //return value;
            //}
        //};

        template<class K, class V>
        class dmap : public std::map<K,V>
        {
            typedef std::map<K,V> BaseType;
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
        struct CategoryNameApp 
          : boost::less_than_comparable<CategoryNameApp> ,boost::equality_comparable<CategoryNameApp>
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
                if(cid<another.cid) return true;
                else if(cid>another.cid) return false;
                else if(depth<another.depth) return true;
                else if(depth>another.depth) return false;
                return false;
            }

            bool operator==(const CategoryNameApp& another) const
            {
                return cid==another.cid&&depth==another.depth;
            }
        };
        struct AttributeApp
          : boost::less_than_comparable<AttributeApp> ,boost::equality_comparable<AttributeApp>
        {
            uint32_t spu_id;
            std::string attribute_name;
            bool is_optional;
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
                //return spu_id==another.spu_id&&attribute_name==another.attribute_name;
                return spu_id==another.spu_id;
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
        struct KeywordTag
        {
            KeywordTag():weight(0.0), aweight(0.0), ngram(1)
            {
            }
            std::vector<CategoryNameApp> category_name_apps;
            std::vector<AttributeApp> attribute_apps;
            std::vector<SpuTitleApp> spu_title_apps;
            double weight; //not serialized, runtime property
            double aweight;
            uint8_t ngram;

            template<class T>
            void SortAndUnique(std::vector<T>& vec)
            {
                std::sort(vec.begin(), vec.end());
                vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
            }
            void Flush()
            {
                SortAndUnique(category_name_apps);
                SortAndUnique(attribute_apps);
                SortAndUnique(spu_title_apps);
            }
            void Append(const KeywordTag& another, bool is_complete)
            {
                uint32_t osize = category_name_apps.size();
                for(uint32_t i=0;i<another.category_name_apps.size();i++)
                {
                    CategoryNameApp aapp = another.category_name_apps[i];
                    aapp.is_complete = is_complete;
                    bool same_cid = false;
                    for(uint32_t j=0;j<osize;j++)
                    {
                        CategoryNameApp& app = category_name_apps[j];
                        if(app.cid==aapp.cid)
                        {
                            if(!app.is_complete&&aapp.is_complete)
                            {
                                app = aapp;
                            }
                            else if( app.is_complete==aapp.is_complete && aapp.depth<app.depth )
                            {
                                app = aapp;
                            }
                            same_cid = true;
                            break;
                        }
                    }
                    if(!same_cid)
                    {
                        category_name_apps.push_back(aapp);
                    }
                    //if(app.find(suffixes[s])==app.end())
                    //{
                        //trie[suffixes[s]].category_name_apps.push_back(cn_app);
                    //}
                    //else
                    //{
                        //std::pair<bool, uint32_t>& last = app[suffixes[s]];
                        //if(! (last.first && !cn_app.is_complete))
                        //{
                            //last.first = cn_app.is_complete;
                            //last.second = cn_app.depth;
                            //trie[suffixes[s]].category_name_apps.back() = cn_app;
                        //}
                    //}
                }
                //category_name_apps.insert(category_name_apps.end(), another.category_name_apps.begin(), another.category_name_apps.end());
                //for(uint32_t i=osize;i<category_name_apps.size();i++)
                //{
                    //if(!is_complete)
                    //{
                        //category_name_apps[i].is_complete = is_complete;
                    //}
                //}
                if(is_complete)
                {
                    attribute_apps.insert(attribute_apps.end(), another.attribute_apps.begin(), another.attribute_apps.end());
                }
                spu_title_apps.insert(spu_title_apps.end(), another.spu_title_apps.begin(), another.spu_title_apps.end());
            }
            KeywordTag& operator^=(const KeywordTag& another)
            {
                category_name_apps.clear();

                uint32_t i=0;
                uint32_t j=0;
                if(ngram>0) //anytime
                {
                    while(i<attribute_apps.size()&&j<another.attribute_apps.size())
                    {
                        AttributeApp& app = attribute_apps[i];
                        const AttributeApp& aapp = another.attribute_apps[j];
                        if(app==aapp)
                        {
                            if(app.attribute_name!=aapp.attribute_name)
                            {
                                app.attribute_name += "+"+aapp.attribute_name;
                                app.is_optional = app.is_optional | aapp.is_optional;
                            }
                            else
                            {
                                app.spu_id = 0;
                            }
                            ++i;
                            ++j;
                        }
                        else if(app<aapp)
                        {
                            app.spu_id = 0;
                            ++i;
                        }
                        else
                        {
                            ++j;
                        }
                    }
                    while(i<attribute_apps.size())
                    {
                        AttributeApp& app = attribute_apps[i];
                        app.spu_id = 0;
                        ++i;
                    }
                }
                i = 0;
                j = 0;
                while(i<spu_title_apps.size()&&j<another.spu_title_apps.size())
                {
                    SpuTitleApp& app = spu_title_apps[i];
                    const SpuTitleApp& aapp = another.spu_title_apps[j];
                    if(app.spu_id==aapp.spu_id)
                    {
                        app.pstart = std::min(app.pstart, aapp.pstart);
                        app.pend = std::max(app.pend, aapp.pend);
                        ++i;
                        ++j;
                    }
                    else if(app<aapp)
                    {
                        app.spu_id = 0;
                        ++i;
                    }
                    else
                    {
                        ++j;
                    }
                }
                while(i<spu_title_apps.size())
                {
                    SpuTitleApp& app = spu_title_apps[i];
                    app.spu_id = 0;
                    ++i;
                }
                //std::vector<AttributeApp> new_attribute_apps;
                //for(uint32_t i=0;i<another.attribute_apps.size();i++)
                //{
                    //const AttributeApp& aapp = another.attribute_apps[i];
                    //for(uint32_t j=0;j<attribute_apps.size();j++)
                    //{
                        //const AttributeApp& app = attribute_apps[j];
                        //if(aapp.spu_id==app.spu_id && aapp.attribute_name!=app.attribute_name)
                        //{
                            //AttributeApp new_app;
                            //new_app.spu_id = app.spu_id;
                            //new_app.attribute_name = app.attribute_name+"+"+aapp.attribute_name;
                            //new_app.is_optional = app.is_optional | aapp.is_optional;
                            //new_attribute_apps.push_back(new_app);
                            //break;
                        //}
                    //}
                //}
                //std::swap(new_attribute_apps, attribute_apps);

                //std::vector<SpuTitleApp> new_spu_title_apps;
                //for(uint32_t i=0;i<another.spu_title_apps.size();i++)
                //{
                    //const SpuTitleApp& aapp = another.spu_title_apps[i];
                    //for(uint32_t j=0;j<spu_title_apps.size();j++)
                    //{
                        //const SpuTitleApp& app = spu_title_apps[j];
                        //if(aapp.spu_id==app.spu_id && aapp.pstart!=app.pstart)
                        //{
                            //SpuTitleApp new_app;
                            //new_app.spu_id = app.spu_id;
                            //new_app.pstart = std::min(app.pstart, aapp.pstart);
                            //new_app.pend = std::max(app.pend, aapp.pend);
                            //new_spu_title_apps.push_back(new_app);
                            //break;
                        //}
                    //}
                //}
                //std::swap(new_spu_title_apps, spu_title_apps);
                if(ngram==1)
                {
                    weight = std::min(weight, another.weight)*3.0;
                }
                else
                {
                    weight = std::min(weight, another.weight)*5.0;
                }
                ++ngram;
                aweight = std::min(aweight, another.aweight)*ngram;
                return *this;
            }
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & category_name_apps & attribute_apps & spu_title_apps;
            }
        };

        typedef std::map<TermList, KeywordTag> KeywordMap;
        typedef std::vector<std::pair<TermList, KeywordTag> > KeywordVector;

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
        };

        struct Product
        {
            std::string spid;
            std::string stitle;
            std::string scategory;
            double price; 
            std::vector<Attribute> attributes;
            double aweight;
            SimObject title_obj;
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & spid & stitle & scategory & price & attributes & aweight;
            }
        };
        typedef uint32_t PidType;
        typedef std::map<std::string, uint32_t> CategoryIndex;
        typedef boost::unordered_map<uint32_t, UString> IdManager;

        ProductMatcher(const std::string& path);
        ~ProductMatcher();
        bool IsOpen() const;
        bool Open();
        static void Clear(const std::string& path);
        bool Index(const std::string& scd_path);
        bool DoMatch(const std::string& scd_path);
        bool Process(const Document& doc, Category& result_category, Product& result_product);

        void SetCmaPath(const std::string& path)
        { cma_path_ = path; }

    private:
        bool PriceMatch_(double p1, double p2);
        double PriceSim_(double offerp, double spup);
        void Analyze_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);
        void AnalyzeChar_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);
        void AnalyzeCR_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);

        void AnalyzeImpl_(idmlib::util::IDMAnalyzer* analyzer, const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);

        bool ProcessBook_(const Document& doc, Product& result_product);


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
        void Compute_(const Document& doc, KeywordVector& keyword_vector, uint32_t& cid, uint32_t& pid);
        uint32_t GetCidBySpuId_(uint32_t spu_id);

        template<class K, class V>
        void GetSortedVector_(const std::map<K, V>& map, std::vector<std::pair<V, K> >& vec)
        {
            for(typename std::map<K,V>::const_iterator it = map.begin();it!=map.end();it++)
            {
                vec.push_back(std::make_pair(it->second, it->first));
            }
            std::sort(vec.begin(), vec.end(), std::greater<std::pair<V, K> >());
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
        CategoryIndex category_index_;
        KeywordSet keyword_set_;
        TrieType trie_;
        KeywordVector keyword_vector_;

        const static double optional_weight_ = 0.2;
        
    };
}

#endif

