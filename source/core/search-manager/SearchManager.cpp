#include "SearchManager.h"
#include "SearchFactory.h"
#include <mining-manager/MiningManager.h>
#include <bundles/index/IndexBundleConfiguration.h>

#include <glog/logging.h>

using namespace sf1r;


// @brief: here SchemaMap is just for normal search, and
// zambeziSearch perparefilter;
SearchManager::SearchManager(
    const IndexBundleConfiguration& config,
    const SearchFactory& searchFactory)
    : preprocessor_(config.indexSchema_)
    , topKReranker_(preprocessor_)
    , fuzzySearchRanker_(preprocessor_)
    , queryBuilder_(searchFactory.createQueryBuilder(
                        preprocessor_.getSchemaMap()))
    , normalSearch_(searchFactory.createSearchBase(
                        SearchingMode::DefaultSearchingMode,
                        preprocessor_, *queryBuilder_))
    , zambeziSearch_(searchFactory.createSearchBase(
                         SearchingMode::ZAMBEZI,
                         preprocessor_, *queryBuilder_))
{
}

void SearchManager::setMiningManager(
    const boost::shared_ptr<MiningManager>& miningManager)
{
    if (!miningManager)
    {
        LOG(WARNING) << "as MininghManager is uninitialized, "
                     << "some functions in SearchManager would not work!";
        return;
    }

    preprocessor_.setProductScorerFactory(
        miningManager->GetProductScorerFactory());

    preprocessor_.setNumericTableBuilder(
        miningManager->GetNumericTableBuilder());

    preprocessor_.setRTypeStringPropTableBuilder(
        miningManager->GetRTypeStringPropTableBuilder());

    topKReranker_.setProductRankerFactory(
        miningManager->GetProductRankerFactory());

    fuzzySearchRanker_.setFuzzyScoreWeight(
        miningManager->getMiningSchema().product_ranking_config);

    fuzzySearchRanker_.enableCategoryClassify(
        miningManager->GetCategoryClassifyTable() != NULL);

    fuzzySearchRanker_.setCustomRankManager(
        miningManager->GetCustomRankManager());

    normalSearch_->setMiningManager(miningManager);

    zambeziSearch_->setMiningManager(miningManager);
}
