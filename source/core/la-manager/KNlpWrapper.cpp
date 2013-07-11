#include "KNlpWrapper.h"
#include <knlp/fmm.h>
#include <knlp/doc_naive_bayes.h>
#include <exception>
#include <glog/logging.h>
#include <boost/filesystem.hpp>

using namespace sf1r;
namespace bfs = boost::filesystem;

KNlpWrapper* KNlpWrapper::get()
{
    return izenelib::util::Singleton<KNlpWrapper>::get();
}

bool KNlpWrapper::initTokenizer(const std::string& dictDir)
{
    const bfs::path dirPath(dictDir);

    try
    {
        tokenizer_.reset(new ilplib::knlp::Fmm(
                             (dirPath / "term.txt").string()));
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", dictDir: " << dictDir;
        return false;
    }

    return true;
}

bool KNlpWrapper::initClassifier(const std::string& dictDir)
{
    const bfs::path dirPath(dictDir);

    try
    {
        cateDict_.reset(new ilplib::knlp::DigitalDictionary(
                            (dirPath / "cate.txt").string()));

        termCateDict_.reset(new ilplib::knlp::DigitalDictionary(
                                (dirPath / "term.cate.txt").string()));

        termMultiCatesDict_.reset(new ilplib::knlp::Dictionary(
                                      (dirPath / "term.multi.cates.txt").string()));
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", dictDir: " << dictDir;
        return false;
    }

    return true;
}

void KNlpWrapper::fmmTokenize(const string_t& str, token_score_list_t& tokenScores)
{
    string_t norm(str);
    ilplib::knlp::Normalize::normalize(norm);

    tokenizer_->fmm(norm, tokenScores);
}

KNlpWrapper::string_t KNlpWrapper::classifyToBestCategory(const token_score_list_t& tokenScores)
{
    string_t category;
    double maxScore = 0;
    category_score_map_t cateScores = classifyToMultiCategories(tokenScores);

    if (cateScores.empty())
        return category;

    category_score_map_t::const_iterator it = cateScores.begin();
    category = it->first;
    maxScore = it->second;

    for (++it; it != cateScores.end(); ++it)
    {
        if (it->second > maxScore)
        {
            category = it->first;
            maxScore = it->second;
        }
    }

    return category;
}

KNlpWrapper::category_score_map_t KNlpWrapper::classifyToMultiCategories(
    const token_score_list_t& tokenScores)
{
    std::stringstream ss;
    return ilplib::knlp::DocNaiveBayes::classify_multi_level(cateDict_.get(),
                                                             termCateDict_.get(),
                                                             termMultiCatesDict_.get(),
                                                             tokenScores,
                                                             ss);
}
