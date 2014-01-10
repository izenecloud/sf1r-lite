#include "ProductTokenizerFactory.h"
#include "CMAProductTokenizer.h"
#include "TrieProductTokenizer.h"
#include "MatcherProductTokenizer.h"
#include "KNlpProductTokenizer.h"
#include "PcaProductTokenizer.h"
#include <common/ResourceManager.h>
#include <la-manager/KNlpDictMonitor.h>
#include <la-manager/TitlePCAWrapper.h>
#include <glog/logging.h>

using namespace sf1r;
namespace bfs = boost::filesystem;

ProductTokenizerFactory::ProductTokenizerFactory(const std::string& resourcePath)
        : dirPath_(bfs::path(resourcePath) / "dict")
{
    typeMap_["fmindex_dic"] = CMA_TOKENIZER;
    typeMap_["product"] = TRIE_TOKENIZER;
    typeMap_["product-matcher"] = MATCHER_TOKENIZER;
    typeMap_["term_category"] = KNLP_TOKENIZER;
    typeMap_["title_pca"] = PCA_TOKENIZER;
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

    case KNLP_TOKENIZER:
        return createKNlpTokenizer_(dictPath);

    case PCA_TOKENIZER:
        return createPcaTokenizer_(dictPath);

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

ProductTokenizer* ProductTokenizerFactory::createKNlpTokenizer_(const std::string& dictPath)
{
    if (!KNlpResourceManager::getResource()->loadDictFiles(dictPath))
        return NULL;

    KNlpDictMonitor::get()->start(dictPath);

    return new KNlpProductTokenizer;
}

ProductTokenizer* ProductTokenizerFactory::createPcaTokenizer_(const std::string& dictPath)
{
    if (!TitlePCAWrapper::get()->loadDictFiles(dictPath))
        return NULL;

    return new PcaProductTokenizer;
}
