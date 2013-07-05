#include "SearchManager.h"
#include "SearchFactory.h"
#include <mining-manager/MiningManager.h>
#include <bundles/index/IndexBundleConfiguration.h>

#include <glog/logging.h>

using namespace sf1r;

SearchManager::SearchManager(
    const IndexBundleConfiguration& config,
    const SearchFactory& searchFactory)
    : preprocessor_(config.indexSchema_)
    , topKReranker_(preprocessor_)
    , fuzzySearchRanker_(preprocessor_)
    , queryBuilder_(searchFactory.createQueryBuilder(
                        preprocessor_.getSchemaMap()))
    , searchBase_(searchFactory.createSearchBase(
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

    searchBase_->setMiningManager(miningManager);
}
