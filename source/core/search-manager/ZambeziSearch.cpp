#include "ZambeziSearch.h"
#include "ZambeziFilter.h"
#include "SearchManagerPreProcessor.h"
#include "Sorter.h"
#include "QueryBuilder.h"
#include "HitQueue.h"
#include <la-manager/AttrTokenizeWrapper.h>
#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/QueryTypeDef.h> // FilteringType
#include <common/ResultType.h>
#include <common/ResourceManager.h>
#include <common/PropSharedLockSet.h>
#include <document-manager/DocumentManager.h>
#include <mining-manager/MiningManager.h>
#include <mining-manager/zambezi-manager/ZambeziManager.h>
#include <mining-manager/group-manager/GroupFilterBuilder.h>
#include <mining-manager/group-manager/GroupFilter.h>
#include <mining-manager/product-scorer/ProductScorer.h>
#include <mining-manager/util/convert_ustr.h>
#include <ir/index_manager/utility/BitVector.h>
#include <util/ClockTimer.h>
#include <glog/logging.h>
#include <iostream>
#include <boost/scoped_ptr.hpp>

namespace
{
const std::size_t kAttrTopDocNum = 200;
const std::size_t kZambeziTopKNum = 1e6;

const std::string kTopLabelPropName = "Category";
const size_t kTopLabelDocNum = 1000;
const size_t kTopLabelCateNum = 4;
}

using namespace sf1r;

ZambeziSearch::ZambeziSearch(
    DocumentManager& documentManager,
    SearchManagerPreProcessor& preprocessor,
    QueryBuilder& queryBuilder)
    : documentManager_(documentManager)
    , preprocessor_(preprocessor)
    , queryBuilder_(queryBuilder)
    , groupFilterBuilder_(NULL)
    , zambeziManager_(NULL)
    , categoryValueTable_(NULL)
{
}

void ZambeziSearch::setMiningManager(
    const boost::shared_ptr<MiningManager>& miningManager)
{
    groupFilterBuilder_ = miningManager->GetGroupFilterBuilder();
    zambeziManager_ = miningManager->getZambeziManager();
    categoryValueTable_ = miningManager->GetPropValueTable(kTopLabelPropName);
}

bool ZambeziSearch::search(
    const SearchKeywordOperation& actionOperation,
    KeywordSearchResult& searchResult,
    std::size_t limit,
    std::size_t offset)
{
    const std::string& query = actionOperation.actionItem_.env_.queryString_;
    LOG(INFO) << "zambezi search for query: " << query;

    if (query.empty())
        return false;

    std::vector<docid_t> candidates;
    std::vector<uint32_t> scores;

    if (!zambeziManager_)
    {
        LOG(WARNING) << "the instance of ZambeziManager is empty";
        return false;
    }

    faceted::GroupParam& groupParam = actionOperation.actionItem_.groupParam_;
    const bool originIsAttrGroup = groupParam.isAttrGroup_;
    groupParam.isAttrGroup_ = false;

    PropSharedLockSet propSharedLockSet;
    boost::scoped_ptr<ProductScorer> productScorer(
        preprocessor_.createProductScorer(actionOperation.actionItem_, propSharedLockSet, NULL));

    boost::shared_ptr<faceted::GroupFilter> groupFilter;
    if (groupFilterBuilder_)
    {
        groupFilter.reset(
            groupFilterBuilder_->createFilter(groupParam, propSharedLockSet));
    }

    const std::vector<QueryFiltering::FilteringType>& filterList =
        actionOperation.actionItem_.filteringList_;
    boost::shared_ptr<InvertedIndexManager::FilterBitmapT> filterBitmap;
    boost::shared_ptr<izenelib::ir::indexmanager::BitVector> filterBitVector;

    if (!filterList.empty())
    {
        queryBuilder_.prepare_filter(filterList, filterBitmap);
        filterBitVector.reset(new izenelib::ir::indexmanager::BitVector);
        filterBitVector->importFromEWAH(*filterBitmap);
    }

    AttrTokenizeWrapper* attrTokenize = AttrTokenizeWrapper::get();
    std::vector<std::pair<std::string, int> > tokenList;
    attrTokenize->attr_tokenize(query, tokenList);

    ZambeziFilter filter(documentManager_, groupFilter, filterBitVector);
    boost::function<bool(uint32_t)> filter_func = boost::bind(&ZambeziFilter::test, &filter, _1);

    {
        boost::shared_lock<boost::shared_mutex> lock(documentManager_.getMutex());
        zambeziManager_->search(tokenList, filter_func, kZambeziTopKNum, candidates, scores);

        if (candidates.empty())
        {
            std::vector<std::pair<std::string, int> > subTokenList;
            attrTokenize->attr_subtokenize(tokenList, subTokenList);
            zambeziManager_->search(subTokenList, filter_func, kZambeziTopKNum, candidates, scores);
        }
    }

    if (candidates.empty())
    {
        LOG(INFO) << "empty search result for query: " << query;
        return false;
    }

    if (candidates.size() != scores.size())
    {
        LOG(WARNING) << "mismatch size of candidate docid and score";
        return false;
    }

    izenelib::util::ClockTimer timer;

    boost::shared_ptr<Sorter> sorter;
    CustomRankerPtr customRanker;
    preprocessor_.prepareSorterCustomRanker(actionOperation,
                                            sorter,
                                            customRanker);

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

    // reset relevance score
    const std::size_t candNum = candidates.size();
    std::size_t totalCount = 0;

    for (size_t i = 0; i < candNum; ++i)
    {
        docid_t docId = candidates[i];
        ScoreDoc scoreItem(docId, scores[i]);
        if (customRanker)
        {
            scoreItem.custom_score = customRanker->evaluate(docId);
        }
        scoreItemQueue->insert(scoreItem);

        ++totalCount;
    }
    /// ret score, add product score;
    std::vector<docid_t> topDocids;
    std::vector<float> topRelevanceScores;
    std::vector<float> topProductScores;

    unsigned int scoreSize = scoreItemQueue->size();
    for (int i = scoreSize - 1; i >= 0; --i)
    {
        const ScoreDoc& scoreItem = scoreItemQueue->pop();
        topDocids.push_back(scoreItem.docId);
        float productScore = 0;
        if (productScorer)
               productScore = productScorer->score(scoreItem.docId);
        topProductScores.push_back(productScore);
        topRelevanceScores.push_back(scoreItem.score);
    }
    zambeziManager_->NormalizeScore(topDocids, topRelevanceScores, topProductScores, propSharedLockSet);

    for (size_t i = 0; i < topRelevanceScores.size(); ++i)
    {
        ScoreDoc scoreItem(topDocids[i], topRelevanceScores[i]);
        scoreItemQueue->insert(scoreItem);
    }
    /// end

    searchResult.totalCount_ = totalCount;
    std::size_t topKCount = 0;
    if (offset < scoreItemQueue->size())// if bigger is zero;
    {
        topKCount = scoreItemQueue->size() - offset;
    }

    std::vector<unsigned int>& docIdList = searchResult.topKDocs_;
    std::vector<float>& rankScoreList = searchResult.topKRankScoreList_;
    std::vector<float>& customScoreList = searchResult.topKCustomRankScoreList_;

    docIdList.resize(topKCount);
    rankScoreList.resize(topKCount);

    if (customRanker)
    {
        customScoreList.resize(topKCount);
    }

    for (int i = topKCount-1; i >= 0; --i)
    {
        const ScoreDoc& scoreItem = scoreItemQueue->pop();
        docIdList[i] = scoreItem.docId;
        rankScoreList[i] = scoreItem.score;
        if (customRanker)
        {
            customScoreList[i] = scoreItem.custom_score;
        }
    }

    if (groupFilter)
    {
        getTopLabels_(docIdList, rankScoreList,
                      propSharedLockSet,
                      searchResult.autoSelectGroupLabels_);

        sf1r::faceted::OntologyRep tempAttrRep;
        groupFilter->getGroupRep(searchResult.groupRep_, tempAttrRep);
    }

    // get attr results for top docs
    if (originIsAttrGroup && groupFilterBuilder_)
    {
        izenelib::util::ClockTimer attrTimer;

        faceted::GroupParam attrGroupParam;
        attrGroupParam.isAttrGroup_ = groupParam.isAttrGroup_ = true;
        attrGroupParam.attrGroupNum_ = groupParam.attrGroupNum_;
        attrGroupParam.searchMode_ = groupParam.searchMode_;

        boost::scoped_ptr<faceted::GroupFilter> attrGroupFilter(
            groupFilterBuilder_->createFilter(attrGroupParam, propSharedLockSet));

        if (attrGroupFilter)
        {
            const size_t topNum = std::min(docIdList.size(), kAttrTopDocNum);
            for (size_t i = 0; i < topNum; ++i)
            {
                attrGroupFilter->test(docIdList[i]);
            }

            faceted::GroupRep tempGroupRep;
            attrGroupFilter->getGroupRep(tempGroupRep, searchResult.attrRep_);
        }

        LOG(INFO) << "attrGroupFilter costs :" << attrTimer.elapsed() << " seconds";
    }

    if (sorter)
    {
        preprocessor_.fillSearchInfoWithSortPropertyData(sorter.get(),
                                                         docIdList,
                                                         searchResult.distSearchInfo_,
                                                         propSharedLockSet);
    }

    LOG(INFO) << "in zambezi ranking, candidate doc num: " << candNum
              << ", costs :" << timer.elapsed() << " seconds";

    return true;
}

void ZambeziSearch::getTopLabels_(
    const std::vector<unsigned int>& docIdList,
    const std::vector<float>& rankScoreList,
    PropSharedLockSet& propSharedLockSet,
    faceted::GroupParam::GroupLabelScoreMap& topLabelMap)
{
    if (!categoryValueTable_)
        return;

    propSharedLockSet.insertSharedLock(categoryValueTable_);

    typedef std::vector<std::pair<faceted::PropValueTable::pvid_t, double> > TopCatIdsT;
    TopCatIdsT topCateIds;
    const std::size_t topNum = std::min(docIdList.size(), kTopLabelDocNum);
    for (std::size_t i = 0; i < topNum; ++i)
    {
        if (topCateIds.size() >= kTopLabelCateNum)
            break;

        category_id_t catId =
            categoryValueTable_->getFirstValueId(docIdList[i]);

        if (catId != 0)
        {
            bool is_exist = false;
            for(TopCatIdsT::const_iterator cit = topCateIds.begin(); cit != topCateIds.end(); ++cit)
            {
                if (cit->first == catId)
                {
                    is_exist = true;
                    break;
                }
            }
            if (!is_exist)
            {
                topCateIds.push_back(std::make_pair(catId, rankScoreList[i]));
            }
        }
    }

    faceted::GroupParam::GroupPathScoreVec& topLabels = topLabelMap[kTopLabelPropName];
    for (TopCatIdsT::const_iterator idIt =
             topCateIds.begin(); idIt != topCateIds.end(); ++idIt)
    {
        std::vector<izenelib::util::UString> ustrPath;
        categoryValueTable_->propValuePath(idIt->first, ustrPath, false);

        std::vector<std::string> path;
        convert_to_str_vector(ustrPath, path);

        topLabels.push_back(std::make_pair(path, idIt->second));
    }

    LOG(INFO) << "top label num: "<< topLabels.size();
}
