#include "ProductScorerFactory.h"
#include "ProductScoreParam.h"
#include "ProductScoreSum.h"
#include "CustomScorer.h"
#include "CategoryScorer.h"
#include "../MiningManager.h"
#include "../custom-rank-manager/CustomRankManager.h"
#include "../product-score-manager/ProductScoreManager.h"
#include <common/PropSharedLockSet.h>
#include <configuration-manager/ProductRankingConfig.h>
#include <memory> // auto_ptr
#include <glog/logging.h>

using namespace sf1r;

namespace
{
/**
 * in order to make the category score less than 10 (the minimum custom
 * score), we would select at most 9 top labels.
 */
const score_t kTopLabelLimit = 9;
}

ProductScorerFactory::ProductScorerFactory(
    const ProductRankingConfig& config,
    MiningManager& miningManager)
    : config_(config)
    , customRankManager_(miningManager.GetCustomRankManager())
    , categoryValueTable_(NULL)
    , productScoreManager_(miningManager.GetProductScoreManager())
{
    const ProductScoreConfig& categoryScoreConfig =
        config.scores[CATEGORY_SCORE];
    const std::string& categoryProp = categoryScoreConfig.propName;
    categoryValueTable_ = miningManager.GetPropValueTable(categoryProp);

    if (categoryValueTable_)
    {
        GroupLabelLogger* clickLogger =
            miningManager.GetGroupLabelLogger(categoryProp);
        const GroupLabelKnowledge* labelKnowledge =
            miningManager.GetGroupLabelKnowledge();

        labelSelector_.reset(new BoostLabelSelector(miningManager,
                                                    *categoryValueTable_,
                                                    clickLogger,
                                                    labelKnowledge));
    }
}

ProductScorer* ProductScorerFactory::createScorer(
    const ProductScoreParam& scoreParam)
{
    std::auto_ptr<ProductScoreSum> scoreSum(new ProductScoreSum);

    bool isany = false;
    for (int i = 0; i < PRODUCT_SCORE_NUM; ++i)
    {
        ProductScorer* scorer = createScorerImpl_(config_.scores[i],
                                                  scoreParam);
        if (scorer)
        {
            scoreSum->addScorer(scorer);
            isany = true;
        }
    }

    if(!isany)
        return NULL;

    return scoreSum.release();
}

ProductScorer* ProductScorerFactory::createScorerImpl_(
    const ProductScoreConfig& scoreConfig,
    const ProductScoreParam& scoreParam)
{
    if (scoreConfig.weight == 0)
        return NULL;

    switch(scoreConfig.type)
    {
    case CUSTOM_SCORE:
        return createCustomScorer_(scoreConfig, scoreParam.query_);

    case CATEGORY_SCORE:
        return createCategoryScorer_(scoreConfig, scoreParam);

    case RELEVANCE_SCORE:
        return createRelevanceScorer_(scoreConfig, scoreParam.relevanceScorer_);

    case POPULARITY_SCORE:
        return createPopularityScorer_(scoreConfig);

    default:
        return NULL;
    }
}

ProductScorer* ProductScorerFactory::createCustomScorer_(
    const ProductScoreConfig& scoreConfig,
    const std::string& query)
{
    if (customRankManager_ == NULL)
        return NULL;

    CustomRankDocId customDocId;
    bool result = customRankManager_->getCustomValue(query, customDocId);

    if (result && !customDocId.topIds.empty())
        return new CustomScorer(scoreConfig, customDocId.topIds);

    return NULL;
}

ProductScorer* ProductScorerFactory::createCategoryScorer_(
    const ProductScoreConfig& scoreConfig,
    const ProductScoreParam& scoreParam)
{
    if (!labelSelector_)
        return NULL;

    std::vector<category_id_t> boostLabels;
    if (labelSelector_->selectLabel(scoreParam, kTopLabelLimit, boostLabels))
    {
        scoreParam.propSharedLockSet_.insertSharedLock(categoryValueTable_);
        return new CategoryScorer(scoreConfig,
                                  *categoryValueTable_,
                                  boostLabels);
    }

    return NULL;
}

ProductScorer* ProductScorerFactory::createRelevanceScorer_(
    const ProductScoreConfig& scoreConfig,
    ProductScorer* relevanceScorer)
{
    if (!relevanceScorer)
        return NULL;

    relevanceScorer->setWeight(scoreConfig.weight);
    return relevanceScorer;
}

ProductScorer* ProductScorerFactory::createPopularityScorer_(
    const ProductScoreConfig& scoreConfig)
{
    if (!productScoreManager_)
        return NULL;

    return productScoreManager_->createProductScorer(scoreConfig);
}
