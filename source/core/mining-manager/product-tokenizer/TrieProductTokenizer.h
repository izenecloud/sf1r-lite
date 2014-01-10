/**
 * @file TrieProductTokenizer.h
 * @brief Trie tokenizer.
 */

#ifndef TRIE_PRODUCT_TOKENIZER_H
#define TRIE_PRODUCT_TOKENIZER_H

#include "ProductTokenizer.h"
#include <am/succinct/ux-trie/uxTrie.hpp>
#include <vector>
#include <string>

namespace sf1r
{

class TrieProductTokenizer : public ProductTokenizer
{
public:
    TrieProductTokenizer(const std::string& dictDir);

    virtual void tokenize(ProductTokenParam& param);

private:
    void GetDictTokens_(
        const std::list<std::string>& input,
        std::list<std::pair<izenelib::util::UString,double> >& tokens,
        std::list<std::string>& left,
        izenelib::am::succinct::ux::Trie* dict_trie,
        double trie_score);

private:
    std::vector<std::pair<std::string, double> > dict_names_;

    std::vector<izenelib::am::succinct::ux::Trie*> tries_;

    friend class TrieProductTokenizerTest;
};

}

#endif // TRIE_PRODUCT_TOKENIZER_H
