#include "ZambeziSearch.h"
#include "SearchManagerPreProcessor.h"
#include "Sorter.h"
#include "QueryBuilder.h"
#include "HitQueue.h"
#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/QueryTypeDef.h> // FilteringType
#include <common/ResultType.h>
#include <common/ResourceManager.h>
#include <common/PropSharedLockSet.h>
#include <mining-manager/MiningManager.h>
#include <mining-manager/zambezi-manager/ZambeziManager.h>
#include <mining-manager/group-manager/GroupFilterBuilder.h>
#include <mining-manager/group-manager/GroupFilter.h>
#include <ir/index_manager/utility/BitVector.h>
#include <util/ClockTimer.h>
#include <glog/logging.h>
#include <iostream>
#include <boost/scoped_ptr.hpp>

namespace
{
const std::size_t kZambeziTopKNum = 1e6;
}

using namespace sf1r;

ZambeziSearch::ZambeziSearch(
    SearchManagerPreProcessor& preprocessor,
    QueryBuilder& queryBuilder)
    : preprocessor_(preprocessor)
    , queryBuilder_(queryBuilder)
    , groupFilterBuilder_(NULL)
    , zambeziManager_(NULL)
{
}

void ZambeziSearch::setMiningManager(
    const boost::shared_ptr<MiningManager>& miningManager)
{
    miningManager_ = miningManager;
    groupFilterBuilder_ = miningManager->GetGroupFilterBuilder();
    zambeziManager_ = miningManager->getZambeziManager();
}

bool ZambeziSearch::search(
    const SearchKeywordOperation& actionOperation,
    KeywordSearchResult& searchResult,
    std::size_t limit,
    std::size_t offset)
{
    const std::string& query = actionOperation.actionItem_.env_.queryString_;
    LOG(INFO) << "zambezi search for query: " << query << endl;

    if (query.empty())
        return false;

    std::vector<docid_t> candidates;
    std::vector<float> scores;

    if (!getTopKDocs_(query, candidates, scores))
        return false;

    rankTopKDocs_(candidates, scores,
                  actionOperation, limit, offset,
                  searchResult);

    return true;
}

bool ZambeziSearch::getTopKDocs_(
    const std::string& query,
    std::vector<docid_t>& candidates,
    std::vector<float>& scores)
{
    if (!zambeziManager_)
    {
        LOG(WARNING) << "the instance of ZambeziManager is empty";
        return false;
    }

    KNlpWrapper::token_score_list_t tokenScores;
    KNlpWrapper::string_t kstr(query);
    boost::shared_ptr<KNlpWrapper> knlpWrapper = KNlpResourceManager::getResource();
    knlpWrapper->fmmTokenize(kstr, tokenScores);

    std::vector<std::string> tokenList;
    std::cout << "tokens:" << std::endl;

    for (KNlpWrapper::token_score_list_t::const_iterator it =
             tokenScores.begin(); it != tokenScores.end(); ++it)
    {
        std::string token = it->first.get_bytes("utf-8");
        tokenList.push_back(token);
        std::cout << token << std::endl;
    }
    std::cout << "-----" << std::endl;

    zambeziManager_->search(tokenList, kZambeziTopKNum, candidates, scores);

    if (candidates.empty())
    {
        LOG(INFO) << "empty search result for query: " << query << endl;
        return false;
    }

    if (candidates.size() != scores.size())
    {
        LOG(WARNING) << "mismatch size of candidate docid and score";
        return false;
    }

    return true;
}

void ZambeziSearch::rankTopKDocs_(
    const std::vector<docid_t>& candidates,
    const std::vector<float>& scores,
    const SearchKeywordOperation& actionOperation,
    std::size_t limit,
    std::size_t offset,
    KeywordSearchResult& searchResult)
{
    izenelib::util::ClockTimer timer;

    boost::shared_ptr<Sorter> sorter;
    CustomRankerPtr customRanker;
    preprocessor_.prepareSorterCustomRanker(actionOperation,
                                            sorter,
                                            customRanker);

    PropSharedLockSet propSharedLockSet;
    boost::scoped_ptr<HitQueue> scoreItemQueue;
    const std::size_t heapSize = limit + offset;

    if (sorter)
    {
        scoreItemQueue.reset(new PropertySortedHitQueue(sorter,
                                                        heapSize,
                                                        propSharedLockSet));
    }
    else
    {
        scoreItemQueue.reset(new ScoreSortedHitQueue(heapSize));
    }

    boost::scoped_ptr<faceted::GroupFilter> groupFilter;
    if (groupFilterBuilder_)
    {
        groupFilter.reset(
            groupFilterBuilder_->createFilter(actionOperation.actionItem_.groupParam_,
                                              propSharedLockSet));
    }

    const std::vector<QueryFiltering::FilteringType>& filterList =
        actionOperation.actionItem_.filteringList_;
    boost::shared_ptr<InvertedIndexManager::FilterBitmapT> filterBitmap;
    boost::scoped_ptr<izenelib::ir::indexmanager::BitVector> filterBitVector;

    if (!filterList.empty())
    {
        queryBuilder_.prepare_filter(filterList, filterBitmap);
        filterBitVector.reset(new izenelib::ir::indexmanager::BitVector);
        filterBitVector->importFromEWAH(*filterBitmap);
    }

    const std::size_t candNum = candidates.size();
    std::size_t totalCount = 0;
    for (size_t i = 0; i < candNum; ++i)
    {
        docid_t docId = candidates[i];
        float score = scores[i];

        if (filterBitVector && !filterBitVector->test(docId))
            continue;

        if (groupFilter && !groupFilter->test(docId))
            continue;

        ScoreDoc scoreItem(docId, score);
        scoreItemQueue->insert(scoreItem);

        ++totalCount;
    }

    searchResult.totalCount_ = totalCount;

    if (groupFilter)
    {
        groupFilter->getGroupRep(searchResult.groupRep_, searchResult.attrRep_);
    }

    std::vector<unsigned int>& docIdList = searchResult.topKDocs_;
    std::vector<float>& rankScoreList = searchResult.topKRankScoreList_;
    std::size_t topKCount = 0;

    if (offset < scoreItemQueue->size())
    {
        topKCount = scoreItemQueue->size() - offset;
    }
    docIdList.resize(topKCount);
    rankScoreList.resize(topKCount);

    for (int i = topKCount-1; i >= 0; --i)
    {
        const ScoreDoc& scoreItem = scoreItemQueue->pop();
        docIdList[i] = scoreItem.docId;
        rankScoreList[i] = scoreItem.score;
    }

    if (sorter)
    {
        preprocessor_.fillSearchInfoWithSortPropertyData(sorter.get(),
                                                         docIdList,
                                                         searchResult.distSearchInfo_);
    }

    LOG(INFO) << "in zambezi ranking, candidate doc num: " << candNum
              << ", total count: " << totalCount
              << ", costs :" << timer.elapsed() << " seconds"
              << std::endl;
}
