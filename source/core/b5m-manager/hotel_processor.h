#ifndef SF1R_B5MMANAGER_HOTELPROCESSOR_H
#define SF1R_B5MMANAGER_HOTELPROCESSOR_H 
#include <util/ustring/UString.h>
#include <product-manager/product_price.h>
#include <document-manager/ScdDocument.h>
#include "b5m_helper.h"
#include "b5m_types.h"
#include "b5m_m.h"
#include <am/sequence_file/ssfr.h>
#include <boost/unordered_map.hpp>
#include <common/ScdWriter.h>
//#include <address-normalizer/address_extract.h>
#include <addrlib/address_extract.h>
NS_SF1R_B5M_BEGIN


class HotelProcessor
{
typedef uint32_t term_t;
typedef std::vector<term_t> word_t;
typedef std::vector<std::size_t> posting_t;
typedef std::map<word_t, posting_t> map_t;
typedef std::pair<std::size_t, std::size_t> DocPair;
typedef std::vector<word_t> WordList;
//typedef boost::unordered_map<DocPair, WordList> MCS;
typedef boost::unordered_map<DocPair, std::size_t> MCS;
typedef std::map<word_t, std::size_t> NoiseSet;
typedef boost::unordered_map<std::size_t, std::vector<std::size_t> > ForwardMap;
struct Item {
    Document doc;
    word_t word;
    std::size_t index;
    word_t prefix_noise_;
    word_t postfix_noise_;
};
template <typename T=std::size_t>
struct PairToForward {
    typedef boost::unordered_map<T, std::size_t> __IdMap;
    typedef boost::unordered_map<T, std::vector<T> > __ForwardMap;
    PairToForward(__IdMap& idmap, __ForwardMap& fmap)
    : idmap_(idmap), fmap_(fmap)
    {
    }

    void Add(const T& i, const T& j)
    {
        __IdMap& idmap = idmap_;
        __ForwardMap& fmap1 = fmap_;
        typename __IdMap::iterator iditi = idmap.find(i);
        typename __IdMap::iterator iditj = idmap.find(j);
        if(iditi==idmap.end()&&iditj==idmap.end())
        {
            std::vector<std::size_t> fv(2);
            fv[0] = i;
            fv[1] = j;
            fmap1[i] = fv;
            idmap[i] = i;
            idmap[j] = i;
        }
        else if(iditi==idmap.end()&&iditj!=idmap.end())
        {
            std::size_t fkey = iditj->second;
            idmap[i] = fkey;
            fmap1[fkey].push_back(i);
        }
        else if(iditi!=idmap.end()&&iditj==idmap.end())
        {
            std::size_t fkey = iditi->second;
            idmap[j] = fkey;
            fmap1[fkey].push_back(j);
        }
        else
        {
            if(iditi->second!=iditj->second)
            {
                std::size_t fkey = iditi->second;
                std::vector<T>& vj = fmap1[iditj->second];
                for(std::size_t k=0;k<vj.size();k++)
                {
                    idmap[vj[k]] = fkey;
                }
                fmap1[fkey].insert(fmap1[fkey].end(), vj.begin(), vj.end());
                vj.clear();
            }
        }
    }
    void TryAddSingle(const T& i)
    {
        typename __IdMap::const_iterator it = idmap_.find(i);
        if(it==idmap_.end())
        {
            idmap_[i] = i;
            fmap_[i].push_back(i);
        }
    }

    void Flush()
    {
        for(typename __ForwardMap::iterator it = fmap_.begin();it!=fmap_.end();++it)
        {
            std::vector<T>& v = it->second;
            std::sort(v.begin(), v.end());
        }
    }

private:
    __IdMap idmap_;
    __ForwardMap& fmap_;
};
typedef izenelib::am::ssf::Writer<std::size_t> Writer;
typedef izenelib::am::ssf::Reader<std::size_t> Reader;
typedef izenelib::am::ssf::Sorter<std::size_t, std::size_t> Sorter;
typedef std::map<word_t, std::size_t> WordCount;
typedef std::map<word_t, double> WordScore;
typedef std::map<word_t, std::string> WordText;
typedef boost::unordered_map<term_t, std::string> TermText;
public:
    HotelProcessor(const B5mM& b5mm);
    ~HotelProcessor();
    void SetDocLimit(std::size_t limit) { limit_ = limit; }
    bool Generate(const std::string& mdb_instance);

private:
    static bool Increase_(WordCount& map, const word_t& word, std::size_t c=1);
    static std::size_t NoiseMatch_(const word_t& word, const NoiseSet& nset, std::size_t& nindex);
    static bool IsSameNoise_(const Item& x, const Item& y);
    void InsertPostfixNoise_(const std::string& word);
    void GetWord_(const std::string& name, word_t& word);
    void DoClustering_(std::vector<Document>& docs);
    double NameSim_(const Item& x, const Item& y, const MCS& mcs);
    double NameSimDetail_(std::size_t x, std::size_t y, std::size_t c);
    double Sim_(const Item& x, const Item& y, const MCS& mcs);
    void ProcessDoc_(ScdDocument& doc);
    void GenP_(const std::vector<Document>& docs, Document& pdoc);
    std::string GetText_(const word_t& word);
    void FindSimilar_(const std::vector<Item>& items, ForwardMap& fmap);
    


private:
    double nsim_threshold_;
    B5mM b5mm_;
    std::size_t limit_;
    idmlib::util::IDMAnalyzer* analyzer_;
    addrlib::AddressExtract* ae_;
    std::string empty_city_;
    boost::shared_ptr<Writer> writer_;
    boost::shared_ptr<ScdWriter> owriter_;
    boost::shared_ptr<ScdWriter> pwriter_;
    NoiseSet postfix_noise_;
    std::size_t postfix_index_;
    TermText term_text_;
    boost::mutex omutex_;
    boost::mutex pmutex_;

};

NS_SF1R_B5M_END

#endif

