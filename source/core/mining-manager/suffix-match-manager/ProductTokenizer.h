#ifndef SF1R_COMMON_PRODUCT_TOKENIZER_H_
#define SF1R_COMMON_PRODUCT_TOKENIZER_H_

#include <util/ustring/UString.h>
#include <util/singleton.h>
#include <am/succinct/ux-trie/uxTrie.hpp>
#include <common/type_defs.h>
#include <string>
#include <list>
#include <map>
#include <vector>

namespace cma
{
class Analyzer;
class Knowledge;
}

namespace sf1r
{
using izenelib::util::UString;

class TrieFactory
{
    std::map<std::string, izenelib::am::succinct::ux::Trie*> tries_;

    int ReadKeyList_(
        const std::string& fn,
        std::vector<std::string>& keyList)
    {
        std::ifstream ifs(fn.c_str());
        if (!ifs)
        {
            std::cerr << "cannot open " << fn << std::endl;
            return -1;
        }

        for (std::string key; getline(ifs, key);)
        {
            if (!key.empty() && key[key.size() - 1] == '\r')
                keyList.push_back(key.substr(0, key.size() - 1));
            else
                keyList.push_back(key);
        }
        return 0;
    }

public:
    TrieFactory() {}
    ~TrieFactory()
    {
        std::map<std::string, izenelib::am::succinct::ux::Trie*>::iterator tit = tries_.begin();
        for (; tit != tries_.end(); ++tit)
            delete tit->second;
    }
    static TrieFactory* Get()
    {
        return izenelib::util::Singleton<TrieFactory>::get();
    }

    izenelib::am::succinct::ux::Trie* GetTrie(const std::string& dictPath)
    {
        std::map<std::string, izenelib::am::succinct::ux::Trie*>::iterator kit = tries_.find(dictPath);
        if (kit != tries_.end())  return kit->second;
        else
        {
            izenelib::am::succinct::ux::Trie* trie = new izenelib::am::succinct::ux::Trie;
            std::vector<std::string> keyList;
            ReadKeyList_(dictPath, keyList);
            trie->build(keyList);
            tries_[dictPath] = trie;
            return trie;
        }
    }

};

class ProductMatcher;
class CategoryClassifyTable;

class ProductTokenizer
{
    enum CharType
    {
        CHAR_CHINESE, ///Chinese
        CHAR_ALNUM, ///Alphebet and digitals
        CHAR_INVALID ///Invalid characters
    };

public:
    enum TokenizerType
    {
        TOKENIZER_CMA, ///type 100 of icma
        TOKENIZER_DICT
    };

    ProductTokenizer(
            TokenizerType type,
            const std::string& dict_path);

    ~ProductTokenizer();

    bool GetTokenResults(
            const std::string& pattern,
            std::list<std::pair<UString, double> >& major_tokens,
            std::list<std::pair<UString, double> >& minor_tokens,
            UString& refined_results);
    bool GetSynonymSet(const UString& pattern, std::vector<UString>& synonym_set, int& setid);
    void SetProductMatcher(ProductMatcher* matcher)
    {
        matcher_ = matcher;
    }

    void GetQuerySumScore(const std::string& pattern, double& sum_score, docid_t docid = 0);

    void setCategoryClassifyTable(CategoryClassifyTable* table);
    
private:
    void Init_(const std::string& dict_path);

    void InitWithCMA_(const std::string& dict_path);

    void InitWithDict_(const std::string& dict_path);

    void InitDict_(const std::string& dict_name);

    double GetTokenResultsByKNlp_(
            const std::string& pattern,
            std::list<std::pair<UString,double> >& token_results,
            UString& refined_results,
            docid_t docid = 0);

    bool GetTokenResultsByCMA_(
            const std::string& pattern,
            std::list<std::pair<UString,double> >& tokens,
            UString& refined_results);

    bool GetTokenResultsByDict_(
            const std::string& pattern,
            std::list<std::pair<UString,double> >& tokens,
            UString& refined_results);

    bool GetTokenResultsByMatcher_(
            const std::string& pattern,
            std::list<std::pair<UString, double> >& major_tokens,
            std::list<std::pair<UString, double> >& minor_tokens,
            UString& refined_results);

    void GetDictTokens_(
            const std::list<std::string>& input,
            std::list<std::pair<UString,double> >& tokens,
            std::list<std::string>& left,
            izenelib::am::succinct::ux::Trie* dict_trie,
            double trie_score);

    void DoBigram_(
            const UString& pattern,
            std::list<std::pair<UString,double> >& tokens,
            double score);

    bool GetLeftTokens_(
            const std::list<std::string>& input,
            std::list<std::pair<UString,double> >& token_results,
            double score = 1.0);

    bool GetLeftTokens_(
            const std::list<UString>& input,
            std::list<std::pair<UString,double> >& token_results,
            double score = 1.0);



    TokenizerType type_;
    std::string dict_path_;

    cma::Analyzer* analyzer_;
    cma::Knowledge* knowledge_;

    ProductMatcher* matcher_;
    CategoryClassifyTable* categoryClassifyTable_;

    std::vector<std::pair<std::string, double> > dict_names_;
    std::vector<izenelib::am::succinct::ux::Trie*> tries_;

    static const UString::CharT SPACE_UCHAR;
    friend class ProductTokenizerTest;
};

}

#endif
