#include "DiversityRoundEvaluator.h"

using namespace sf1r;

DiversityRoundEvaluator::DiversityRoundEvaluator()
    : ProductScoreEvaluator("round")
    , lastCategoryScore_(0)
{
}

score_t DiversityRoundEvaluator::evaluate(ProductScore& productScore)
{
    const merchant_id_t singleMerchantId = productScore.singleMerchantId_;
    const std::vector<score_t>& rankScores = productScore.rankScores_;

    // ignore multiple merchants
    if (singleMerchantId == 0 || rankScores.empty())
        return 0;

    // reset the round status when "category score" is changed
    const score_t categoryScore = rankScores.front();
    if (lastCategoryScore_ != categoryScore)
    {
        roundMap_.clear();
        lastCategoryScore_ = categoryScore;
    }

    return --roundMap_[singleMerchantId];
}
