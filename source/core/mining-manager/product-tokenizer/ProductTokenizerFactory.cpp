#include "ProductTokenizerFactory.h"
#include "CMAProductTokenizer.h"
#include "TrieProductTokenizer.h"
#include "MatcherProductTokenizer.h"
#include <glog/logging.h>

using namespace sf1r;
namespace bfs = boost::filesystem;

ProductTokenizerFactory::ProductTokenizerFactory(const std::string& resourcePath)
        : dirPath_(bfs::path(resourcePath) / "dict")
{
    typeMap_["fmindex_dic"] = CMA_TOKENIZER;
    typeMap_["product"] = TRIE_TOKENIZER;
    typeMap_["product-matcher"] = MATCHER_TOKENIZER;
}

ProductTokenizer* ProductTokenizerFactory::createProductTokenizer(const std::string& dirName)
{
    TokenizerType type = getTokenizerType_(dirName);
    const std::string dictPath = (dirPath_ / dirName).string();

    switch (type)
    {
    case CMA_TOKENIZER:
        return createCMATokenizer_(dictPath);

    case TRIE_TOKENIZER:
        return createTrieTokenizer_(dictPath);

    case MATCHER_TOKENIZER:
        return createMatcherTokenizer_();

    default:
        LOG(WARNING) << "unknown product dictionary name " << dirName;
        return NULL;
    }
}

ProductTokenizerFactory::TokenizerType ProductTokenizerFactory::getTokenizerType_(const std::string& dirName) const
{
    TypeMap::const_iterator it = typeMap_.find(dirName);

    if (it != typeMap_.end())
        return it->second;

    return TOKENIZER_NUM;
}

ProductTokenizer* ProductTokenizerFactory::createCMATokenizer_(const std::string& dictPath)
{
    return new CMAProductTokenizer(dictPath);
}

ProductTokenizer* ProductTokenizerFactory::createTrieTokenizer_(const std::string& dictPath)
{
    return new TrieProductTokenizer(dictPath);
}

ProductTokenizer* ProductTokenizerFactory::createMatcherTokenizer_()
{
    return new MatcherProductTokenizer;
}
