#include "ProductRankerFactory.h"
#include "ProductRanker.h"
#include "CategoryScoreEvaluator.h"
#include "MerchantCountEvaluator.h"
#include "DiversityRoundEvaluator.h"
#include "../MiningManager.h"
#include "../group-manager/PropValueTable.h"
#include <memory> // auto_ptr
#include <glog/logging.h>

using namespace sf1r;

ProductRankerFactory::ProductRankerFactory(MiningManager& miningManager)
    : config_(miningManager.getMiningSchema().product_ranking_config)
    , merchantValueTable_(NULL)
{
    const std::string& merchantProp =
        config_.scores[DIVERSITY_SCORE].propName;

    merchantValueTable_ = miningManager.GetPropValueTable(merchantProp);
}

ProductRanker* ProductRankerFactory::createProductRanker(ProductRankParam& param)
{
    if (merchantValueTable_ == NULL)
    {
        LOG(WARNING) << "failed to create ProductRanker";
        return NULL;
    }

    std::auto_ptr<ProductRanker> ranker(
        new ProductRanker(param, config_.isDebug));

    ranker->addEvaluator(new CategoryScoreEvaluator(config_));
    ranker->addEvaluator(new MerchantCountEvaluator(*merchantValueTable_));
    ranker->addEvaluator(new DiversityRoundEvaluator);

    return ranker.release();
}
