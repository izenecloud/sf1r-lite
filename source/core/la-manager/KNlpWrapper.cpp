#include "KNlpWrapper.h"
#include <knlp/fmm.h>
#include <knlp/doc_naive_bayes.h>
#include <knlp/string_patterns.h>
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

        garbagePattern_.reset(new ilplib::knlp::GarbagePattern(
                                  (dirPath / "garbage.pat").string()));
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

        termMultiCatesDict_.reset(new ilplib::knlp::VectorDictionary(
                                      (dirPath / "term.multi.cates.txt").string()));

        termMultiCatesCondDict_.reset(new ilplib::knlp::VectorDictionary(
                                          (dirPath / "term.multi.cates.cond.txt").string()));

        originalToClassifyCateDict_.reset(new ilplib::knlp::Dictionary(
                                              (dirPath / "tcate_map.txt").string()));
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

void KNlpWrapper::fmmBigram(std::vector<std::pair<KString,double> >& r)
{
    tokenizer_->bigram(r);
}

void KNlpWrapper::fmmBigram_with_space(std::vector<std::pair<KString,double> >& r)
{
    tokenizer_->bigram_with_space(r);
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
                                                             termMultiCatesDict_.get(),
                                                             termMultiCatesCondDict_.get(),
                                                             tokenScores,
                                                             ss);
}

std::string KNlpWrapper::mapFromOriginalCategory(const std::string& originalCategory)
{
    string_t kstr(originalCategory);
    const char* classifyCategory = originalToClassifyCateDict_->value(kstr, true);

    if (classifyCategory == NULL)
        return std::string();

    return classifyCategory;
}

std::string KNlpWrapper::cleanGarbage(const std::string& str)
{
    return garbagePattern_->clean(str);
}

std::string KNlpWrapper::cleanStopword(const std::string& str)
{
    return garbagePattern_->erase_stop_word(str);
}
