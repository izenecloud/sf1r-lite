/**
 * @file ProductTokenizerFactory.h
 * @brief create product tokenizer instances.
 */

#ifndef PRODUCT_TOKENIZER_FACTORY_H
#define PRODUCT_TOKENIZER_FACTORY_H

#include <string>
#include <map>
#include <boost/filesystem.hpp>

namespace sf1r
{
class ProductTokenizer;

class ProductTokenizerFactory
{
public:
    ProductTokenizerFactory(const std::string& resourcePath);

    /**
     * @param dirName such as "fmindex_dic", "product", "product-matcher", etc.
     */
    ProductTokenizer* createProductTokenizer(const std::string& dirName);

private:
    enum TokenizerType
    {
        CMA_TOKENIZER = 0,
        TRIE_TOKENIZER,
        MATCHER_TOKENIZER,
        KNLP_TOKENIZER,
        PCA_TOKENIZER,
        TOKENIZER_NUM
    };

    TokenizerType getTokenizerType_(const std::string& dirName) const;

    ProductTokenizer* createCMATokenizer_(const std::string& dictPath);

    ProductTokenizer* createTrieTokenizer_(const std::string& dictPath);

    ProductTokenizer* createMatcherTokenizer_();

    ProductTokenizer* createKNlpTokenizer_();

    ProductTokenizer* createPcaTokenizer_(const std::string& dictPath);

private:
    const boost::filesystem::path dirPath_;

    /** key: dict dir */
    typedef std::map<std::string, TokenizerType> TypeMap;
    TypeMap typeMap_;
};

}

#endif // PRODUCT_TOKENIZER_FACTORY_H
