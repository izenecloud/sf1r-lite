#include "CMAProductTokenizer.h"
#include <common/CMAKnowledgeFactory.h>
#include <glog/logging.h>

using namespace sf1r;

CMAProductTokenizer::CMAProductTokenizer(const std::string& dictDir)
        : knowledge_(NULL)
        , analyzer_(NULL)
{
    knowledge_ = CMAKnowledgeFactory::Get()->GetKnowledge(dictDir, false);
    analyzer_ = cma::CMA_Factory::instance()->createAnalyzer();

    analyzer_->setOption(cma::Analyzer::OPTION_TYPE_POS_TAGGING, 0);
    // using the maxprefix analyzer
    analyzer_->setOption(cma::Analyzer::OPTION_ANALYSIS_TYPE, 100);
    analyzer_->setKnowledge(knowledge_);
    LOG(INFO) << "load CMA dictionary knowledge finished.";
}

CMAProductTokenizer::~CMAProductTokenizer()
{
    delete analyzer_;
}

void CMAProductTokenizer::tokenize(ProductTokenParam& param)
{
    cma::Sentence sentence(param.query.c_str());
    analyzer_->runWithSentence(sentence);

    ProductTokenParam::TokenScoreList& minorTokens = param.minorTokens;
    ProductTokenParam::TokenScore tokenScore;

    // tokenize by maxprefix match in dictionary
    tokenScore.second = 2;
    for (int i = 0; i < sentence.getCount(0); ++i)
    {
        tokenScore.first.assign(sentence.getLexicon(0, i),
                                izenelib::util::UString::UTF_8);
        minorTokens.push_back(tokenScore);
    }

    if (param.isRefineResult)
    {
        for (ProductTokenParam::TokenScoreListIter it = minorTokens.begin();
             it != minorTokens.end(); ++it)
        {
            param.refinedResult.append(it->first);
            param.refinedResult.push_back(SPACE_UCHAR);
        }
    }

    // tokenize by maxprefix match in bigram
    tokenScore.second = 1;
    for (int i = 0; i < sentence.getCount(1); i++)
    {
        tokenScore.first.assign(sentence.getLexicon(1, i),
                                izenelib::util::UString::UTF_8);
        minorTokens.push_back(tokenScore);
    }
}
