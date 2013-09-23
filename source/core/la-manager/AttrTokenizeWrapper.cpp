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

std::vector<std::pair<std::string, double> >
    AttrTokenizeWrapper::attr_tokenize_index(const std::string& title, const std::string& attr, 
                    const std::string& cate, const std::string& ocate,
                    const std::string& source)
{
    return attr_tokenizer_->tokenize(title, attr, cate, ocate, source);
}

std::vector<std::string> AttrTokenizeWrapper::attr_tokenize(const std::string& Q)
{
    return attr_tokenizer_->tokenize(Q);
}

std::vector<std::string> AttrTokenizeWrapper::attr_subtokenize(const std::vector<std::string>& tks)
{
    return attr_tokenizer_->subtokenize(tks);
}

std::vector<char*>** AttrTokenizeWrapper::get_TermCategory(const std::string& query)
{
    return queryMultiCatesDict_->value(KString(query), true);
}
