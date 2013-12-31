#include "TitlePCAWrapper.h"
#include <exception>
#include <glog/logging.h>
#include <util/singleton.h>
#include <boost/filesystem.hpp>
#include <knlp/title_pca.h>


using namespace sf1r;
namespace bfs = boost::filesystem;

TitlePCAWrapper::TitlePCAWrapper()
    : isDictLoaded_(false)
{

}

TitlePCAWrapper::~TitlePCAWrapper()
{

}

TitlePCAWrapper* TitlePCAWrapper::get()
{
    return izenelib::util::Singleton<TitlePCAWrapper>::get();   
}

bool TitlePCAWrapper::loadDictFiles(const std::string& dictDir)
{
    if (isDictLoaded_)
        return true;

    dictDir_ = dictDir;
    LOG(INFO) << "Start loading title_pca dictionaries in " << dictDir_;
    const bfs::path dirPath(dictDir_);

    try
    {
        LOG(INFO) << "load title_pca" ;
        title_pca_.reset(new ilplib::knlp::TitlePCA(dirPath.string()));
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", dictDir: " << dictDir_;
        return false;
    }

    isDictLoaded_ = true;
    LOG(INFO) << "Finished loading title_pca.";

    return true;
}

void TitlePCAWrapper::pca(std::string line, 
        std::vector<std::pair<std::string, float> >& tks, 
        std::string& brand, 
        std::string& model_type,
        std::vector<std::pair<std::string, float> >& sub_tks, bool do_sub = false)const
{
    title_pca_->pca(line, tks, brand, model_type, sub_tks, do_sub);
}

