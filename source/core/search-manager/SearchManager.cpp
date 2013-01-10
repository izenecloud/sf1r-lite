#include "SearchManager.h"
#include "SearchThreadParam.h"
#include "CustomRanker.h"
#include "Sorter.h"
#include "HitQueue.h"
#include "ScoreDocEvaluator.h"

#include <bundles/index/IndexBundleConfiguration.h>
#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/ActionItem.h>
#include <common/ResultType.h>
#include <common/PropSharedLockSet.h>
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
    : productRankerFactory_(NULL)
    , fuzzyScoreWeight_(0)
    , topKReranker_(preprocessor_)
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

    topKReranker_.setProductRankerFactory(
        miningManager->GetProductRankerFactory());

    preprocessor_.productScorerFactory_ =
        miningManager->GetProductScorerFactory();

    preprocessor_.numericTableBuilder_ =
        miningManager->GetNumericTableBuilder();

    searchThreadWorker_->setGroupFilterBuilder(
        miningManager->GetGroupFilterBuilder());

    searchThreadWorker_->setCustomRankManager(
        miningManager->GetCustomRankManager());

    fuzzyScoreWeight_ = getFuzzyScoreWeight_(miningManager);
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

void SearchManager::rankDocIdListForFuzzySearch(const SearchKeywordOperation& actionOperation,
        uint32_t start, std::vector<uint32_t>& docid_list, std::vector<float>& result_score_list,
        std::vector<float>& custom_score_list)
{
    if(docid_list.empty())
        return;
    CustomRankerPtr customRanker;
    boost::shared_ptr<Sorter> pSorter;
    try
    {
        preprocessor_.prepare_sorter_customranker_(actionOperation,
                                                    customRanker,
                                                    pSorter);
    }
    catch (std::exception& e)
    {
        return;
    }

    PropSharedLockSet propSharedLockSet;
    ProductScorer* productScorer = preprocessor_.createProductScorer(
        actionOperation.actionItem_, propSharedLockSet, NULL);

    if (productScorer == NULL && !customRanker &&
       preprocessor_.isSortByRankProp(actionOperation.actionItem_.sortPriorityList_))
    {
        LOG(INFO) << "no need to resort, sorting by original fuzzy match order.";
        return;
    }

    ScoreDocEvaluator scoreDocEvaluator(productScorer, customRanker);
    const std::size_t count = docid_list.size();
    result_score_list.resize(count);
    custom_score_list.resize(count);

    boost::scoped_ptr<HitQueue> scoreItemQueue;
    if (pSorter)
    {
        ///sortby
        scoreItemQueue.reset(new PropertySortedHitQueue(pSorter,
                                                        count,
                                                        propSharedLockSet));
    }
    else
    {
        scoreItemQueue.reset(new ScoreSortedHitQueue(count));
    }

    ScoreDoc tmpdoc;
    for(size_t i = 0; i < count; ++i)
    {
        tmpdoc.docId = docid_list[i];
        scoreDocEvaluator.evaluate(tmpdoc);

        float fuzzyScore = result_score_list[i];
        if(productScorer == NULL)
        {
            tmpdoc.score = fuzzyScore;
        }
        else
        {
            tmpdoc.score += fuzzyScore * fuzzyScoreWeight_;
        }

        scoreItemQueue->insert(tmpdoc);
        //cout << "doc : " << tmpdoc.docId << ", score is:" << tmpdoc.score << "," << tmpdoc.custom_score << endl;
    }
    const std::size_t need_count = start > 0 ? (scoreItemQueue->size() - start) : scoreItemQueue->size();
    docid_list.resize(need_count);
    result_score_list.resize(need_count);
    custom_score_list.resize(need_count);
    for (size_t i = 0; i < need_count; ++i)
    {
        const ScoreDoc& pScoreItem = scoreItemQueue->pop();
        docid_list[need_count - i - 1] = pScoreItem.docId;
        result_score_list[need_count - i - 1] = pScoreItem.score;
        if (customRanker)
        {
            custom_score_list[need_count - i - 1] = pScoreItem.custom_score;
        }
    }
}

score_t SearchManager::getFuzzyScoreWeight_(
    const boost::shared_ptr<MiningManager>& miningManager) const
{
    const MiningSchema& miningSchema = miningManager->getMiningSchema();
    const ProductScoreConfig& fuzzyConfig =
        miningSchema.product_ranking_config.scores[FUZZY_SCORE];

    return fuzzyConfig.weight;
}

} // namespace sf1r
