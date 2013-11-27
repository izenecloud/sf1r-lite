#include "ZambeziTokenizer.h"
#include <common/CMAKnowledgeFactory.h>
#include <common/ResourceManager.h>
#include <common/QueryNormalizer.h>
#include <boost/filesystem.hpp>
#include <util/singleton.h>
#include <glog/logging.h>
#include <iostream>

using namespace cma;

namespace sf1r
{
ZambeziTokenizer::ZambeziTokenizer()
    : isInit_(false)
    , isItemUnique_(true)
    , type_(ZambeziTokenizer::CMA_MAXPRE)
    , analyzer_(NULL)
    , knowledge_(NULL)
{
}

ZambeziTokenizer::~ZambeziTokenizer()
{
    if (analyzer_) delete analyzer_;
}

void ZambeziTokenizer::setItemUnique(bool isUnique)
{
    isItemUnique_ = isUnique;
}

void ZambeziTokenizer::initWithCMA_(TokenizerType type, const std::string& dict_path)
{
    if (isInit_)
        return;
    isInit_ = true;

    LOG(INFO) << dict_path ;
    knowledge_ = CMAKnowledgeFactory::Get()->GetKnowledge(dict_path, false);

    analyzer_ = CMA_Factory::instance()->createAnalyzer();
    analyzer_->setOption(Analyzer::OPTION_TYPE_POS_TAGGING, 0);
    // using the maxprefix analyzer
    if(type_ == CMA_MAXPRE)
    {
        LOG(INFO) << "use CMA_MAXPRE";
        analyzer_->setOption(Analyzer::OPTION_ANALYSIS_TYPE, 100);//CMA_MAXPRE;
    }
    else if (type_ == CMA_FMINCOVER)
    {
        LOG(INFO) << "use CMA_FMINCOVER";
        analyzer_->setOption(Analyzer::OPTION_ANALYSIS_TYPE, 5);//fmincover;
    }
    analyzer_->setKnowledge(knowledge_);
}

bool ZambeziTokenizer::getTokenResults(
        const std::string& pattern,
        std::vector<std::pair<std::string, int> >& token_results)
{
        return getTokenResultsByCMA_(pattern,
                                     token_results);
}

//make sure the term is unique;
bool ZambeziTokenizer::getTokenResultsByCMA_(
        const std::string& pattern,
        std::vector<std::pair<string, int> >& token_results)
{
    std::set<std::pair<string, int> > token_set;
    Sentence pattern_sentence(pattern.c_str());
    analyzer_->runWithSentence(pattern_sentence);

    if (!isItemUnique_)
    {
        for (int i = 0; i < pattern_sentence.getCount(0); ++i)
            token_results.push_back(std::make_pair(pattern_sentence.getLexicon(0, i), 2));
        for (int i = 0; i < pattern_sentence.getCount(1); i++)
            token_results.push_back(std::make_pair(pattern_sentence.getLexicon(1, i), 1));

        return true;
    }

    //LOG(INFO) << "query tokenize by maxprefix match in dictionary: ";
    for (int i = 0; i < pattern_sentence.getCount(0); ++i)
    {
        if(!token_set.insert(std::make_pair(pattern_sentence.getLexicon(0, i), 2)).second)
            token_set.insert(std::make_pair(pattern_sentence.getLexicon(0, i), 2));
    }

    //LOG(INFO) << "query tokenize by maxprefix match in bigram: ";
    for (int i = 0; i < pattern_sentence.getCount(1); i++)
    {
        if(!token_set.insert(std::make_pair(pattern_sentence.getLexicon(1, i), 1)).second)
            token_set.insert(std::make_pair(pattern_sentence.getLexicon(1, i), 1));
    }
    
    for (std::set<std::pair<string, int> >::iterator i = token_set.begin(); i != token_set.end(); ++i)
    {
        token_results.push_back(*i);
    }
    return true;
}

}
