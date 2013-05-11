#include "FuzzySearchRanker.h"
#include "SearchManagerPreProcessor.h"
#include "CustomRanker.h"
#include "Sorter.h"
#include "HitQueue.h"
#include "ScoreDocEvaluator.h"

#include <configuration-manager/ProductRankingConfig.h>
#include <query-manager/SearchKeywordOperation.h>
#include <common/PropSharedLockSet.h>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

using namespace sf1r;

FuzzySearchRanker::FuzzySearchRanker(SearchManagerPreProcessor& preprocessor)
    : preprocessor_(preprocessor)
    , fuzzyScoreWeight_(0)
{
}

void FuzzySearchRanker::setFuzzyScoreWeight(const ProductRankingConfig& rankConfig)
{
    const ProductScoreConfig& fuzzyConfig = rankConfig.scores[FUZZY_SCORE];
    fuzzyScoreWeight_ = fuzzyConfig.weight;
}

void FuzzySearchRanker::rank(
        const SearchKeywordOperation& actionOperation,
        uint32_t start,
        std::vector<uint32_t>& docid_list,
        std::vector<float>& result_score_list,
        std::vector<float>& custom_score_list)
{
    if (docid_list.size() <= start)
        return;

    CustomRankerPtr customRanker;
    boost::shared_ptr<Sorter> pSorter;
    try
    {
        preprocessor_.prepareSorterCustomRanker(actionOperation,
                                                pSorter,
                                                customRanker);
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
    for (size_t i = 0; i < count; ++i)
    {
        tmpdoc.docId = docid_list[i];
        scoreDocEvaluator.evaluate(tmpdoc);

        float fuzzyScore = result_score_list[i];
        if (!productScorer)
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

    const std::size_t need_count = scoreItemQueue->size() - start;
    docid_list.resize(need_count);
    result_score_list.resize(need_count);

    if (customRanker)
    {
        custom_score_list.resize(need_count);
    }

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
