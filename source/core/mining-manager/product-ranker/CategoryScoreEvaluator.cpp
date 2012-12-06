#include "CategoryScoreEvaluator.h"
#include <configuration-manager/ProductRankingConfig.h>
#include <algorithm> // min

using namespace sf1r;

namespace
{

score_t extractWeight(const ProductRankingConfig& config)
{
    score_t customWeight = config.scores[CUSTOM_SCORE].weight;
    score_t categoryWeight = config.scores[CATEGORY_SCORE].weight;

    if (customWeight == 0)
        return categoryWeight;

    if (categoryWeight == 0)
        return customWeight;

    return std::min(customWeight, categoryWeight);
}

}

CategoryScoreEvaluator::CategoryScoreEvaluator(const ProductRankingConfig& config)
    : ProductScoreEvaluator("category")
    , weight_(extractWeight(config))
{
}

score_t CategoryScoreEvaluator::evaluate(ProductScore& productScore)
{
    if (weight_ == 0)
        return 0;

    score_t result = productScore.topKScore_ / weight_;
    return static_cast<int>(result);
}
