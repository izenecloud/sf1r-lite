#include "KNlpWrapper.h"
#include <knlp/fmm.h>
#include <knlp/doc_naive_bayes.h>
#include <knlp/string_patterns.h>
#include <knlp/maxent/maxent_classify.h>
#include <exception>
#include <glog/logging.h>
#include <boost/filesystem.hpp>

using namespace sf1r;
namespace bfs = boost::filesystem;

KNlpWrapper* KNlpWrapper::get()
{
    return izenelib::util::Singleton<KNlpWrapper>::get();
}

KNlpWrapper::KNlpWrapper()
    : isDictLoaded_(false)
{
}

void KNlpWrapper::setDictDir(const std::string& dictDir)
{
    dictDir_ = dictDir;
}

bool KNlpWrapper::loadDictFiles()
{
    if (isDictLoaded_)
        return true;

    LOG(INFO) << "Start loading knlp dictionaries in " << dictDir_;
    const bfs::path dirPath(dictDir_);

    try
    {
        tokenizer_.reset(new ilplib::knlp::Fmm(
                             (dirPath / "term.txt").string()));

        garbagePattern_.reset(new ilplib::knlp::GarbagePattern(
                                  (dirPath / "garbage.pat").string()));

        cateDict_.reset(new ilplib::knlp::DigitalDictionary(
                            (dirPath / "cate.txt").string()));

        termMultiCatesDict_.reset(new ilplib::knlp::VectorDictionary(
                                      (dirPath / "term.multi.cates.txt").string()));

        termMultiCatesCondDict_.reset(new ilplib::knlp::VectorDictionary(
                                          (dirPath / "term.multi.cates.cond.txt").string()));

        originalToClassifyCateDict_.reset(new ilplib::knlp::Dictionary(
                                              (dirPath / "tcate_map.txt").string()));

        longStrClassifier_.reset(new ilplib::knlp::MaxentClassify(
                                  (dirPath / "maxent.model").string(),
                                  tokenizer_.get(),
                                  garbagePattern_.get()));

        shortStrClassifier_.reset(new ilplib::knlp::MaxentClassify(
                                    (dirPath / "maxent.query.model").string(),
                                    tokenizer_.get(),
                                    garbagePattern_.get(),
                                    (dirPath / "query.dict").string()));
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", dictDir: " << dictDir_;
        return false;
    }

    isDictLoaded_ = true;
    LOG(INFO) << "Finished loading knlp dictionaries.";

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

KNlpWrapper::category_score_map_t KNlpWrapper::classifyToMultiCategories(
    const std::string& str,
    bool isLongStr)
{
    std::stringstream ss;
    return isLongStr ?
        longStrClassifier_->classify(str, ss, true) :
        shortStrClassifier_->classify(str, ss, true, 1);
}

std::string KNlpWrapper::getBestCategory(const category_score_map_t& categoryScoreMap)
{
    std::string category;
    double maxScore = 0;

    if (categoryScoreMap.empty())
        return category;

    category_score_map_t::const_iterator it = categoryScoreMap.begin();
    category = it->first;
    maxScore = it->second;

    for (++it; it != categoryScoreMap.end(); ++it)
    {
        if (it->second > maxScore)
        {
            category = it->first;
            maxScore = it->second;
        }
    }

    return category;
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

void KNlpWrapper::gauss_smooth(std::vector<double>& r)
{
    tokenizer_->gauss_smooth(r);
}
