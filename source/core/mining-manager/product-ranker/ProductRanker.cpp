#include "ProductRanker.h"
#include "ProductScorer.h"
#include "ProductReranker.h"
#include "ProductRankingParam.h"
#include "ProductRankerFactory.h"

#include <glog/logging.h>
#include <algorithm> // sort
#include <iostream>

namespace
{
const std::size_t PRINT_DOC_LIMIT = 20;
}

namespace sf1r
{

ProductRanker::ProductRanker(
    ProductRankingParam& param,
    ProductRankerFactory& rankerFactory,
    bool isDebug
)
    : rankingParam_(param)
    , rankerFactory_(rankerFactory)
    , isDebug_(isDebug)
{
}

void ProductRanker::rank()
{
    if (! rankingParam_.isValid())
    {
        LOG(ERROR) << "invalid parameter for product ranking";
        return;
    }

    loadScore_();

    sortScore_();

    getResult_();
}

void ProductRanker::loadScore_()
{
    const std::size_t docNum = rankingParam_.docNum_;
    scoreMatrix_.reserve(docNum);

    for (std::size_t i = 0; i < docNum; ++i)
    {
        scoreMatrix_.push_back(
            ProductScoreList(rankingParam_.docIds_[i], rankingParam_.relevanceScores_[i]));
    }

    std::vector<ProductScorer*>& scorers = rankerFactory_.getScorers();

    for (std::vector<ProductScorer*>::iterator scorerIt = scorers.begin();
        scorerIt != scorers.end(); ++scorerIt)
    {
        (*scorerIt)->pushScore(rankingParam_, scoreMatrix_);
    }

    printMatrix_("the matrix after loading score:");
}

void ProductRanker::sortScore_()
{
    std::sort(scoreMatrix_.begin(), scoreMatrix_.end());

    ProductReranker* diversityReranker =
        rankerFactory_.getReranker(ProductRankerFactory::DIVERSITY_RERANKER);

    if (diversityReranker)
    {
        diversityReranker->rerank(scoreMatrix_);
    }

    printMatrix_("the matrix after sorting score:");
}

void ProductRanker::getResult_()
{
    for (std::size_t i = 0; i < rankingParam_.docNum_; ++i)
    {
        const ProductScoreList& scoreList = scoreMatrix_[i];

        rankingParam_.docIds_[i] = scoreList.docId_;
        rankingParam_.relevanceScores_[i] = scoreList.relevanceScore_;
    }
}

void ProductRanker::printMatrix_(const std::string& message) const
{
    if (! isDebug_)
        return;

    std::cout << message << std::endl;

    std::cout << "docid\t";
    std::vector<ProductScorer*>& scorers = rankerFactory_.getScorers();
    for (std::vector<ProductScorer*>::const_iterator it = scorers.begin();
        it != scorers.end(); ++it)
    {
        std::cout << (*it)->getScoreMessage() << "\t";
    }
    std::cout << "mid\tround" << std::endl;

    std::size_t printDocNum = std::min(scoreMatrix_.size(), PRINT_DOC_LIMIT);
    for (std::size_t i = 0; i < printDocNum; ++i)
    {
        const ProductScoreList& scoreList = scoreMatrix_[i];
        std::cout << scoreList.docId_ << "\t";

        const std::vector<score_t>& rankingScores = scoreList.rankingScores_;
        for (std::vector<score_t>::const_iterator it = rankingScores.begin();
            it != rankingScores.end(); ++it)
        {
            std::cout << *it << "\t";
        }
        std::cout << scoreList.singleMerchantId_ << "\t"
                  << scoreList.diversityRound_ << std::endl;
    }

    std::cout << std::endl;
}

} // namespace sf1r
