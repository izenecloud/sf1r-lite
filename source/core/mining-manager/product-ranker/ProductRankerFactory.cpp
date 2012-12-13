#include "ProductRankerFactory.h"
#include "ProductRanker.h"
#include "ProductRankParam.h"
#include "CategoryScoreEvaluator.h"
#include "MerchantCountEvaluator.h"
#include "DiversityRoundEvaluator.h"
#include "RandomScoreEvaluator.h"
#include "../group-manager/PropValueTable.h"
#include <configuration-manager/ProductRankingConfig.h>
#include <memory> // auto_ptr
#include <algorithm> // min

using namespace sf1r;

namespace
{

score_t minCustomCategoryWeight(const ProductRankingConfig& config)
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

ProductRankerFactory::ProductRankerFactory(
    const ProductRankingConfig& config,
    const faceted::PropValueTable* merchantValueTable)
    : config_(config)
    , merchantValueTable_(merchantValueTable)
    , isRandomScoreConfig_(config.scores[RANDOM_SCORE].weight != 0)
{
}

ProductRanker* ProductRankerFactory::createProductRanker(ProductRankParam& param)
{
    std::auto_ptr<ProductRanker> ranker(
        new ProductRanker(param, config_.isDebug));

    addCategoryEvaluator_(*ranker);

    if (param.isRandomRank_)
    {
        addRandomEvaluator_(*ranker);
    }
    else
    {
        addDiversityEvaluator_(*ranker);
    }

    return ranker.release();
}

void ProductRankerFactory::addCategoryEvaluator_(ProductRanker& ranker) const
{
    const score_t minWeight = minCustomCategoryWeight(config_);

    ranker.addEvaluator(new CategoryScoreEvaluator(minWeight));
}

void ProductRankerFactory::addRandomEvaluator_(ProductRanker& ranker) const
{
    if (!isRandomScoreConfig_)
        return;

    ranker.addEvaluator(new RandomScoreEvaluator);
}

void ProductRankerFactory::addDiversityEvaluator_(ProductRanker& ranker) const
{
    if (!merchantValueTable_)
        return;

    ranker.addEvaluator(new MerchantCountEvaluator(*merchantValueTable_));
    ranker.addEvaluator(new DiversityRoundEvaluator);
}
