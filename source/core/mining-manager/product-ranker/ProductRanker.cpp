#include "ProductRanker.h"
#include "ProductRankParam.h"
#include "ProductScoreEvaluator.h"

#include <glog/logging.h>
#include <algorithm> // stable_sort
#include <iostream>

namespace
{
const std::size_t kPrintDocLimit = 24;
}

using namespace sf1r;

ProductRanker::ProductRanker(
    ProductRankParam& param,
    bool isDebug)
    : rankParam_(param)
    , isDebug_(isDebug)
{
}

ProductRanker::~ProductRanker()
{
    for (std::vector<ProductScoreEvaluator*>::iterator it = evaluators_.begin();
         it != evaluators_.end(); ++it)
    {
        delete *it;
    }
}

void ProductRanker::addEvaluator(ProductScoreEvaluator* evaluator)
{
    evaluators_.push_back(evaluator);
}

void ProductRanker::rank()
{
    if (!rankParam_.isValid())
    {
        LOG(ERROR) << "invalid parameter for product ranking";
        return;
    }

    loadScore_();

    sortScore_();

    fetchResult_();
}

void ProductRanker::loadScore_()
{
    const std::size_t docNum = rankParam_.docNum_;
    scoreList_.reserve(docNum);

    for (std::size_t i = 0; i < docNum; ++i)
    {
        ProductScore score(rankParam_.docIds_[i],
                           rankParam_.topKScores_[i],
                           docNum);
        evaluateScore_(score);
        scoreList_.push_back(score);
    }

    printScore_("evaluated scores:");
}

void ProductRanker::evaluateScore_(ProductScore& productScore)
{
    for (std::vector<ProductScoreEvaluator*>::iterator it = evaluators_.begin();
        it != evaluators_.end(); ++it)
    {
        score_t score = (*it)->evaluate(productScore);
        productScore.rankScores_.push_back(score);
    }
}

void ProductRanker::sortScore_()
{
    std::stable_sort(scoreList_.begin(), scoreList_.end());

    printScore_("sorted scores:");
}

void ProductRanker::fetchResult_()
{
    for (std::size_t i = 0; i < rankParam_.docNum_; ++i)
    {
        const ProductScore& productScore = scoreList_[i];

        rankParam_.docIds_[i] = productScore.docId_;
        rankParam_.topKScores_[i] = productScore.topKScore_;
    }
}

void ProductRanker::printScore_(const std::string& banner) const
{
    if (!isDebug_)
        return;

    std::cout << banner << std::endl;
    std::cout << "docid\t";
    for (std::vector<ProductScoreEvaluator*>::const_iterator it =
             evaluators_.begin(); it != evaluators_.end(); ++it)
    {
        std::cout << (*it)->getScoreName() << "\t";
    }
    std::cout << "mid\ttopKScore" << std::endl;

    std::size_t printDocNum = std::min(scoreList_.size(), kPrintDocLimit);
    for (std::size_t i = 0; i < printDocNum; ++i)
    {
        const ProductScore& productScore = scoreList_[i];
        std::cout << productScore.docId_ << "\t";

        const std::vector<score_t>& rankScores = productScore.rankScores_;
        for (std::vector<score_t>::const_iterator it = rankScores.begin();
            it != rankScores.end(); ++it)
        {
            std::cout << *it << "\t";
        }
        std::cout << productScore.singleMerchantId_ << "\t"
                  << productScore.topKScore_ << std::endl;
    }

    std::cout << std::endl;
}
