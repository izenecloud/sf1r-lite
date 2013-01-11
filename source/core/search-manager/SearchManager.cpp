#include "SearchManager.h"
#include "SearchThreadParam.h"

#include <bundles/index/IndexBundleConfiguration.h>
#include <query-manager/SearchKeywordOperation.h>
#include <common/ResultType.h>
#include <mining-manager/MiningManager.h>

#include <glog/logging.h>

namespace sf1r
{

SearchManager::SearchManager(
    const IndexBundleSchema& indexSchema,
    const boost::shared_ptr<IDManager>& idManager,
    const boost::shared_ptr<DocumentManager>& documentManager,
    const boost::shared_ptr<IndexManager>& indexManager,
    const boost::shared_ptr<RankingManager>& rankingManager,
    IndexBundleConfiguration* config)
    : topKReranker_(preprocessor_)
    , fuzzySearchRanker_(preprocessor_)
{
    for (IndexBundleSchema::const_iterator iter = indexSchema.begin();
            iter != indexSchema.end(); ++iter)
    {
        preprocessor_.schemaMap_[iter->getName()] = *iter;
    }

    queryBuilder_.reset(new QueryBuilder(
                            indexManager,
                            documentManager,
                            idManager,
                            rankingManager,
                            preprocessor_.schemaMap_,
                            config->filterCacheNum_));

    searchThreadWorker_.reset(new SearchThreadWorker(*config,
                                                     documentManager,
                                                     indexManager,
                                                     rankingManager,
                                                     preprocessor_,
                                                     *queryBuilder_));

    searchThreadMaster_.reset(new SearchThreadMaster(*config,
                                                     preprocessor_,
                                                     documentManager,
                                                     *searchThreadWorker_));
}

void SearchManager::reset_filter_cache()
{
    queryBuilder_->reset_cache();
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

    preprocessor_.productScorerFactory_ =
        miningManager->GetProductScorerFactory();

    preprocessor_.numericTableBuilder_ =
        miningManager->GetNumericTableBuilder();

    searchThreadWorker_->setGroupFilterBuilder(
        miningManager->GetGroupFilterBuilder());

    searchThreadWorker_->setCustomRankManager(
        miningManager->GetCustomRankManager());

    topKReranker_.setProductRankerFactory(
        miningManager->GetProductRankerFactory());

    fuzzySearchRanker_.setFuzzyScoreWeight(
        miningManager->getMiningSchema().product_ranking_config);
}

bool SearchManager::search(
    const SearchKeywordOperation& actionOperation,
    KeywordSearchResult& searchResult,
    std::size_t limit,
    std::size_t offset)
{
    if (!actionOperation.noError())
        return false;

    const std::size_t heapSize = limit + offset;
    std::vector<SearchThreadParam> threadParams;
    DistKeywordSearchInfo& distSearchInfo = searchResult.distSearchInfo_;

    searchThreadMaster_->prepareThreadParams(actionOperation,
                                             distSearchInfo,
                                             heapSize,
                                             threadParams);

    if (!searchThreadMaster_->runThreadParams(threadParams))
        return false;

    if (distSearchInfo.isOptionGatherInfo())
        return true;

    bool result = searchThreadMaster_->mergeThreadParams(threadParams) &&
                  searchThreadMaster_->fetchSearchResult(offset,
                                                         threadParams.front(),
                                                         searchResult);
    REPORT_PROFILE_TO_SCREEN();
    return result;
}

} // namespace sf1r
