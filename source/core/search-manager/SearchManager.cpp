#include "SearchManager.h"
#include "NormalSearch.h"
#include <mining-manager/MiningManager.h>
#include <bundles/index/IndexBundleConfiguration.h>

#include <glog/logging.h>

namespace sf1r
{

SearchManager::SearchManager(
    const IndexBundleConfiguration& config,
    const boost::shared_ptr<IDManager>& idManager,
    const boost::shared_ptr<DocumentManager>& documentManager,
    const boost::shared_ptr<IndexManager>& indexManager,
    const boost::shared_ptr<RankingManager>& rankingManager)
    : preprocessor_(config.indexSchema_)
    , topKReranker_(preprocessor_)
    , fuzzySearchRanker_(preprocessor_)
{
    queryBuilder_.reset(new QueryBuilder(
                            indexManager,
                            documentManager,
                            idManager,
                            rankingManager,
                            preprocessor_.getSchemaMap(),
                            config.filterCacheNum_));

    searchBase_.reset(new NormalSearch(config,
                                       documentManager,
                                       indexManager,
                                       rankingManager,
                                       preprocessor_,
                                       *queryBuilder_));
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

    searchBase_->setMiningManager(miningManager);

    topKReranker_.setProductRankerFactory(
        miningManager->GetProductRankerFactory());

    fuzzySearchRanker_.setFuzzyScoreWeight(
        miningManager->getMiningSchema().product_ranking_config);
}

} // namespace sf1r
