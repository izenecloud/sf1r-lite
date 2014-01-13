
#include "AttrTokenizeWrapper.h"
#include <knlp/attr_tokenize.h>
#include <knlp/dictionary.h>
#include <exception>
#include <glog/logging.h>
#include <util/singleton.h>
#include <boost/filesystem.hpp>

using namespace sf1r;
namespace bfs = boost::filesystem;

AttrTokenizeWrapper::AttrTokenizeWrapper()
    : isDictLoaded_(false)
{

}

AttrTokenizeWrapper::~AttrTokenizeWrapper()
{

}

AttrTokenizeWrapper* AttrTokenizeWrapper::get()
{
    return izenelib::util::Singleton<AttrTokenizeWrapper>::get();   
}

bool AttrTokenizeWrapper::loadDictFiles(const std::string& dictDir)
{
    if (isDictLoaded_)
        return true;

    dictDir_ = dictDir;
    LOG(INFO) << "Start loading attr_tokenize dictionaries in " << dictDir_;
    const bfs::path dirPath(dictDir_);

    try
    {
        attr_tokenizer_.reset(new ilplib::knlp::AttributeTokenize( dirPath.string() ));
        LOG(INFO) << "load /single_term2cate.mapping" ;
        std::string queryCate = "/single_term2cate.mapping";
        queryMultiCatesDict_.reset(new ilplib::knlp::VectorDictionary(dictDir_ + queryCate));
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", dictDir: " << dictDir_;
        return false;
    }

    isDictLoaded_ = true;
    LOG(INFO) << "Finished loading attr_tokenize dictionaries.";

    return true;
}

void AttrTokenizeWrapper::attr_tokenize_index(
    const std::string& title, 
    const std::string& attr, 
    const std::string& cate, 
    const std::string& ocate,
    const std::string& source,
    std::vector<std::pair<std::string, double> >& tokenScoreList)
{
    attr_tokenizer_->tokenize(title, attr, cate, ocate, source, tokenScoreList);
}

void AttrTokenizeWrapper::attr_tokenize(
    const std::string& Q, 
    std::vector<std::pair<std::string, int> >& tokenList)
{
    attr_tokenizer_->tokenize(Q, tokenList);
}

bool AttrTokenizeWrapper::attr_subtokenize(
        const std::vector<std::pair<std::string, int> >& tks,
        std::vector<std::pair<std::string, int> >& tokenList)
{
    attr_tokenizer_->subtokenize(tks, tokenList);
    return tokenList != tks;
}

double AttrTokenizeWrapper::att_name_weight(
    const std::string& attr_name,
    const std::string& cate)
{
    return attr_tokenizer_->att_weight(attr_name, cate);
}

double AttrTokenizeWrapper::att_value_weight(
    const std::string& attr_name,
    const std::string& attr_value,
    const std::string& cate)
{
    return attr_tokenizer_->att_weight(attr_name, attr_value, cate);
}

std::vector<char*>** AttrTokenizeWrapper::get_TermCategory(const std::string& query)
{
    if (!queryMultiCatesDict_)
        return NULL;
    return queryMultiCatesDict_->value(KString(query), true);
}
