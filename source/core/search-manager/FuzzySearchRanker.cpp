#include "FuzzySearchRanker.h"
#include "SearchManagerPreProcessor.h"
#include "CustomRanker.h"
#include "Sorter.h"
#include "HitQueue.h"

#include <configuration-manager/ProductRankingConfig.h>
#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/ActionItem.h>
#include <common/PropSharedLockSet.h>
#include <mining-manager/product-scorer/ProductScorer.h>
#include "mining-manager/custom-rank-manager/CustomRankManager.h"

#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

using namespace sf1r;

FuzzySearchRanker::FuzzySearchRanker(SearchManagerPreProcessor& preprocessor)
    : preprocessor_(preprocessor)
    , fuzzyScoreWeight_(0)
    , isCategoryClassify_(false)
    , customRankManager_(NULL)
{
}

void FuzzySearchRanker::setFuzzyScoreWeight(const ProductRankingConfig& rankConfig)
{
    const ProductScoreConfig& fuzzyConfig = rankConfig.scores[FUZZY_SCORE];
    fuzzyScoreWeight_ = fuzzyConfig.weight;
}

void FuzzySearchRanker::rankByProductScore(
    const KeywordSearchActionItem& actionItem,
    std::vector<ScoreDocId>& resultList)
{
    PropSharedLockSet propSharedLockSet;
    boost::scoped_ptr<ProductScorer> productScorer(
        preprocessor_.createProductScorer(actionItem, propSharedLockSet, NULL));

    if (!productScorer)
        return;

    std::set<docid_t> excludeDocIds;
    getExcludeDocIds_(actionItem.env_.normalizedQueryString_,
                      excludeDocIds);

    const std::size_t count = resultList.size();
    std::size_t current = 0;
    for (std::size_t i = 0; i < count; ++i)
    {
        docid_t docId = resultList[i].second;

        // ignore the exclude docids
        if (excludeDocIds.find(docId) != excludeDocIds.end())
            continue;

        double fuzzyScore = resultList[i].first;
        double productScore = productScorer->score(docId);

        if (isCategoryClassify_)
        {
            // ignore the docs with zero category score
            if (productScore < 0.00009)
                continue;

            fuzzyScore = static_cast<int>(fuzzyScore * fuzzyScoreWeight_);
        }

        resultList[i].first = fuzzyScore + productScore;
        resultList[current++] = resultList[i];
    }

    resultList.resize(current);
    std::sort(resultList.begin(), resultList.end(), std::greater<ScoreDocId>());

    if (current == count)
    {
        LOG(INFO) << "rank by product score, topk count not changed: " << count;
    }
    else
    {
        LOG(INFO) << "rank by product score, topk count changed from "
                  << count << " to " << current;
    }
}

void FuzzySearchRanker::getExcludeDocIds_(const std::string& query,
                                          std::set<docid_t>& excludeDocIds)
{
    if (customRankManager_ == NULL)
        return;

    CustomRankDocId customDocId;
    if (customRankManager_->getCustomValue(query, customDocId))
    {
        excludeDocIds.insert(customDocId.excludeIds.begin(),
                             customDocId.excludeIds.end());
    }
}

void FuzzySearchRanker::rankByPropValue(
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

    if (!customRanker &&
        preprocessor_.isSortByRankProp(actionOperation.actionItem_.sortPriorityList_))
    {
        LOG(INFO) << "no need to resort, sorting by original fuzzy match order.";
        return;
    }

    const std::size_t count = docid_list.size();
    PropSharedLockSet propSharedLockSet;
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
        tmpdoc.score = result_score_list[i];

        if (customRanker)
        {
            tmpdoc.custom_score = customRanker->evaluate(tmpdoc.docId);
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
