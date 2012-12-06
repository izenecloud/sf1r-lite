#include "ProductRankerFactory.h"
#include "ProductRanker.h"
#include "CategoryScoreEvaluator.h"
#include "MerchantCountEvaluator.h"
#include "DiversityRoundEvaluator.h"
#include "../group-manager/PropValueTable.h"
#include <configuration-manager/ProductRankingConfig.h>
#include <memory> // auto_ptr
#include <glog/logging.h>

using namespace sf1r;

ProductRankerFactory::ProductRankerFactory(
    const ProductRankingConfig& config,
    const faceted::PropValueTable& merchantValueTable)
    : config_(config)
    , merchantValueTable_(merchantValueTable)
{
}

ProductRanker* ProductRankerFactory::createProductRanker(ProductRankParam& param)
{
    std::auto_ptr<ProductRanker> ranker(
        new ProductRanker(param, config_.isDebug));

    ranker->addEvaluator(new CategoryScoreEvaluator(config_));
    ranker->addEvaluator(new MerchantCountEvaluator(merchantValueTable_));
    ranker->addEvaluator(new DiversityRoundEvaluator);

    return ranker.release();
}
