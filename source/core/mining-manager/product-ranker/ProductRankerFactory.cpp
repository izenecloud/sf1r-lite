#include "ProductRankerFactory.h"
#include "ProductRanker.h"
#include "TopKCustomRankScorer.h"
#include "CategoryScorer.h"
#include "MerchantScorer.h"
#include "RelevanceScorer.h"
#include "MerchantDiversityReranker.h"
#include "CTRReranker.h"
#include "../MiningManager.h"
#include "../group-manager/PropValueTable.h"
#include "../group-manager/GroupManager.h"
#include "../group-label-logger/GroupLabelLogger.h"
#include "../faceted-submanager/ctr_manager.h"
#include "../merchant-score-manager/MerchantScoreManager.h"
#include <search-manager/SearchManager.h>

namespace sf1r
{

ProductRankerFactory::ProductRankerFactory(MiningManager* miningManager)
    : miningManager_(miningManager)
    , config_(miningManager_->GetMiningSchema().product_ranking_config)
    , categoryValueTable_(NULL)
    , merchantValueTable_(NULL)
    , merchantScoreManager_(miningManager_->GetMerchantScoreManager())
    , rerankers_(RERANKER_TYPE_NUM)
    , categoryScorer_(NULL)
{
    const std::string& categoryProp = config_.categoryPropName;
    const std::string& merchantProp = config_.merchantPropName;

    const faceted::GroupManager* groupManager = miningManager_->GetGroupManager();
    if (groupManager)
    {
        categoryValueTable_ = groupManager->getPropValueTable(categoryProp);
        merchantValueTable_ = groupManager->getPropValueTable(merchantProp);
    }

    createTopKCustomRankScorer_();
    createCategoryScorer_();
    createMerchantScorer_();
    createRelevanceScorer_();
    createCTRReranker_();
}

ProductRankerFactory::~ProductRankerFactory()
{
    for (std::vector<ProductScorer*>::iterator it = scorers_.begin();
        it != scorers_.end(); ++it)
    {
        delete *it;
    }

    for (std::vector<ProductReranker*>::iterator it = rerankers_.begin();
        it != rerankers_.end(); ++it)
    {
        delete *it;
    }
}

ProductRanker* ProductRankerFactory::createProductRanker(ProductRankingParam& param)
{
    if (categoryScorer_)
    {
        categoryScorer_->createCategoryScores(param);
    }

    return new ProductRanker(param, *this, config_.isDebug);
}

void ProductRankerFactory::createTopKCustomRankScorer_()
{
    scorers_.push_back(new TopKCustomRankScorer);
}

void ProductRankerFactory::createCategoryScorer_()
{
    if (! categoryValueTable_)
        return;

    GroupLabelLogger* categoryClickLogger =
        miningManager_->GetGroupLabelLogger(config_.categoryPropName);

    boost::shared_ptr<SearchManager>& searchManager =
        miningManager_->GetSearchManager();

    const std::vector<std::string>& scorePropNames =
        config_.categoryScorePropNames;

    categoryScorer_ = new CategoryScorer(categoryValueTable_,
        categoryClickLogger, searchManager, scorePropNames);

    scorers_.push_back(categoryScorer_);
}

void ProductRankerFactory::createMerchantScorer_()
{
    if (!merchantValueTable_ || !merchantScoreManager_)
        return;

    ProductScorer* merchantScorer = new MerchantScorer(
        categoryValueTable_, merchantValueTable_, merchantScoreManager_);

    const int compareScoreCount = scorers_.size();
    scorers_.push_back(merchantScorer);

    rerankers_[DIVERSITY_RERANKER] = new MerchantDiversityReranker(compareScoreCount);
}

void ProductRankerFactory::createRelevanceScorer_()
{
    scorers_.push_back(new RelevanceScorer);
}

void ProductRankerFactory::createCTRReranker_()
{
    boost::shared_ptr<faceted::CTRManager> ctrManager = miningManager_->GetCtrManager();

    if (ctrManager)
    {
        rerankers_[CTR_RERANKER] = new CTRReranker(ctrManager);
    }
}

} // namespace sf1r
