#include "ZambeziTokenizer.h"
#include <common/CMAKnowledgeFactory.h>
#include <common/ResourceManager.h>
#include <common/QueryNormalizer.h>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <iostream>

using namespace cma;

namespace sf1r
{
ZambeziTokenizer::ZambeziTokenizer(
    TokenizerType type,
    const std::string& dict_path)
    : type_(type)
    , analyzer_(NULL)
    , knowledge_(NULL)
{
    InitWithCMA_(dict_path);
}

ZambeziTokenizer::~ZambeziTokenizer()
{
    if (analyzer_) delete analyzer_;
}

void ZambeziTokenizer::InitWithCMA_(const std::string& dict_path)
{
    LOG(INFO) << "CMAKnowledgeFactory::Get()->GetKnowledge(dict_path, false);";
    knowledge_ = CMAKnowledgeFactory::Get()->GetKnowledge(dict_path, false);

    LOG(INFO) << "END";
    analyzer_ = CMA_Factory::instance()->createAnalyzer();
    analyzer_->setOption(Analyzer::OPTION_TYPE_POS_TAGGING, 0);
    // using the maxprefix analyzer
    if(type_ == CMA_MAXPRE)
        analyzer_->setOption(Analyzer::OPTION_ANALYSIS_TYPE, 100);//CMA_MAXPRE;
    else if (type_ = CMA_FMINCOVER)
        analyzer_->setOption(Analyzer::OPTION_ANALYSIS_TYPE, 5);//fmincover;
    
    analyzer_->setKnowledge(knowledge_);
    LOG(INFO) << "load dictionary knowledge finished.";
}

bool ZambeziTokenizer::GetTokenResults(
        const std::string& pattern,
        std::list<std::pair<std::string, double> >& token_results)
{
        return GetTokenResultsByCMA_(pattern,
                                     token_results);
}

bool ZambeziTokenizer::GetTokenResultsByCMA_(
        const std::string& pattern,
        std::list<std::pair<string, double> >& token_results)
{
    Sentence pattern_sentence(pattern.c_str());

    analyzer_->runWithSentence(pattern_sentence);
    //LOG(INFO) << "query tokenize by maxprefix match in dictionary: ";
    for (int i = 0; i < pattern_sentence.getCount(0); ++i)
    {
        token_results.push_back(std::make_pair(pattern_sentence.getLexicon(0, i), (double)2.0));
    }

    //LOG(INFO) << "query tokenize by maxprefix match in bigram: ";
    for (int i = 0; i < pattern_sentence.getCount(1); i++)
    {
        //printf("%s, ", pattern_sentence.getLexicon(1, i));
        token_results.push_back(std::make_pair(pattern_sentence.getLexicon(1, i), (double)1.0));
    }
    //cout << endl;

    return true;
}

}
