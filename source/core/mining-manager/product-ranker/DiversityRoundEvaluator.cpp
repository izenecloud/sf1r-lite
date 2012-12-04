#include "DiversityRoundEvaluator.h"

using namespace sf1r;

DiversityRoundEvaluator::DiversityRoundEvaluator()
    : ProductScoreEvaluator("round")
    , lastCategoryScore_(0)
{
}

score_t DiversityRoundEvaluator::evaluate(ProductScore& productScore)
{
    const std::vector<score_t>& rankScores = productScore.rankScores_;

    if (rankScores.size() < 2)
        return 0;

    score_t categoryScore = rankScores[0];
    score_t merchantCount = rankScores[1];

    // ignore multiple merchants
    if (merchantCount != 1)
        return 0;

    // reset the round status when "category score" is changed
    if (lastCategoryScore_ != categoryScore)
    {
        roundMap_.clear();
        lastCategoryScore_ = categoryScore;
    }

    return --roundMap_[productScore.singleMerchantId_];
}
