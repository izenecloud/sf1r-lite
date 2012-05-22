#include "ProductRanker.h"
#include "ProductScorer.h"
#include "ProductReranker.h"
#include "ProductRankingParam.h"
#include "ProductRankerFactory.h"

#include <glog/logging.h>
#include <algorithm> // sort

namespace sf1r
{

ProductRanker::ProductRanker(
    ProductRankingParam& param,
    ProductRankerFactory& rankerFactory
)
    : rankingParam_(param)
    , rankerFactory_(rankerFactory)
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

} // namespace sf1r
