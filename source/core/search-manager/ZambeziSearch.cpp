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
#include <index-manager/zambezi-manager/ZambeziManager.h>
#include <mining-manager/group-manager/GroupFilterBuilder.h>
#include <mining-manager/group-manager/GroupFilter.h>
#include <mining-manager/product-scorer/ProductScorer.h>
#include <mining-manager/util/convert_ustr.h>
#include <b5m-manager/product_matcher.h>
#include <ir/index_manager/utility/BitVector.h>
#include <util/ClockTimer.h>
#include <glog/logging.h>
#include <iostream>
#include <set>
#include <boost/scoped_ptr.hpp>

namespace sf1r
{

namespace
{
const std::size_t kAttrTopDocNum = 200;
const std::size_t kZambeziTopKNum = 1e6;

const std::string kTopLabelPropName = "Category";
const size_t kRootCateNum = 10;

const izenelib::util::UString::CharT kUCharSpace = ' ';

const MonomorphicFilter<true> kAllPassFilter;
}

bool DocLess(const ScoreDoc& o1, const ScoreDoc& o2)
{
    return (o1.score > o2.score);
}

ZambeziSearch::ZambeziSearch(
    DocumentManager& documentManager,
    SearchManagerPreProcessor& preprocessor,
    QueryBuilder& queryBuilder,
    ZambeziManager* zambeziManager)
    : documentManager_(documentManager)
    , preprocessor_(preprocessor)
    , queryBuilder_(queryBuilder)
    , groupFilterBuilder_(NULL)
    , zambeziManager_(zambeziManager)
    , categoryValueTable_(NULL)
{
}

void ZambeziSearch::setMiningManager(
    const boost::shared_ptr<MiningManager>& miningManager)
{
    groupFilterBuilder_ = miningManager->GetGroupFilterBuilder();
    categoryValueTable_ = miningManager->GetPropValueTable(kTopLabelPropName);
    attrManager_= miningManager->GetAttributeManager();
    numericTableBuilder_ = miningManager->GetNumericTableBuilder();
}

void ZambeziSearch::normalizeTopDocs_(
    const boost::scoped_ptr<ProductScorer>& productScorer,
    boost::scoped_ptr<HitQueue>& scoreItemQueue,
    std::vector<ScoreDoc>& resultList,
    PropSharedLockSet &sharedLockSet)
{
    izenelib::util::ClockTimer timer;
    std::vector<docid_t> topDocids;
    std::vector<float> topRelevanceScores;
    std::vector<float> topProductScores;

    unsigned int scoreSize = scoreItemQueue->size();
    for (int i = scoreSize - 1; i >= 0; --i)
    {
        const ScoreDoc& scoreItem = scoreItemQueue->pop();
        topDocids.push_back(scoreItem.docId);
        float productScore = 0;
        productScore = productScorer->score(scoreItem.docId);
        topProductScores.push_back(productScore);
        if (!zambeziManager_->isAttrTokenize())
            topRelevanceScores.push_back(scoreItem.score + productScore);
        else
            topRelevanceScores.push_back(scoreItem.score);
    }

    //Normalize for attr_tokens
    if (zambeziManager_->isAttrTokenize())
        normalizeScore_(topDocids, topRelevanceScores, topProductScores, sharedLockSet);

    // fast sort
    resultList.resize(topRelevanceScores.size());
    for (size_t i = 0; i < topRelevanceScores.size(); ++i)
    {
        ScoreDoc scoreItem(topDocids[i], topRelevanceScores[i]);
        resultList[i] = scoreItem;
    }
    std::sort(resultList.begin(), resultList.end(), DocLess);  //desc
    LOG(INFO) << " Use productScore, Normalize TOP "<< resultList.size()
            << "Docs cost:" << timer.elapsed() << " seconds";
}

bool ZambeziSearch::search(
    const SearchKeywordOperation& actionOperation,
    KeywordSearchResult& searchResult,
    std::size_t limit,
    std::size_t offset)
{
    const std::vector<std::string>& search_in_properties = actionOperation.actionItem_.searchPropertyList_;
    const std::string& query = actionOperation.actionItem_.env_.queryString_;

    izenelib::ir::Zambezi::Algorithm algorithm;
    getZambeziAlgorithm(actionOperation.actionItem_.searchingMode_.algorithm_, algorithm);

    LOG(INFO) << "zambezi search for query: " << query;
    if (query.empty())
        return false;

    std::vector<docid_t> candidates;
    std::vector<float> scores;

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
    //Query Analyzer
    getAnalyzedQuery_(query, searchResult.analyzedQuery_);

    std::vector<std::pair<std::string, int> > tokenList;

    if (zambeziManager_->isAttrTokenize())
        AttrTokenizeWrapper::get()->attr_tokenize(query, tokenList); // kevin'dict 
    else
        zambeziManager_->getTokenizer()->getTokenResults(query, tokenList);
    
    zambeziManager_->search(algorithm, tokenList, kZambeziTopKNum,
                            search_in_properties, candidates, scores);

    if (candidates.empty() && zambeziManager_->isAttrTokenize())
    {
        std::vector<std::pair<std::string, int> > subTokenList;
        AttrTokenizeWrapper::get()->attr_subtokenize(tokenList, subTokenList);

        zambeziManager_->search(algorithm, subTokenList, kZambeziTopKNum, 
                                search_in_properties, candidates, scores);
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

    {
        ZambeziFilter filter(documentManager_, groupFilter, filterBitVector);

        for (size_t i = 0; i < candNum; ++i)
        {
            docid_t docId = candidates[i];

            if (!filter.test(docId))
                continue;

            ScoreDoc scoreItem(docId, scores[i]);
            if (customRanker)
            {
                scoreItem.custom_score = customRanker->evaluate(docId);
            }
            scoreItemQueue->insert(scoreItem);

            ++totalCount;
        }
    }

    std::vector<ScoreDoc> resultList;
    unsigned int scoreSize = scoreItemQueue->size();
    if (!sorter && productScorer) // productScorer; //!sorter
    {
        LOG(INFO) << "do normalize top docs ...";
        normalizeTopDocs_(productScorer, 
                        scoreItemQueue, 
                        resultList,
                        propSharedLockSet);
    }
    else
    {
        resultList.resize(scoreSize);
        for (int i = scoreSize - 1; i >= 0; --i)
        {
            const ScoreDoc& scoreItem = scoreItemQueue->pop();
            resultList[i] = scoreItem; 
        }
    }

    searchResult.totalCount_ = totalCount;
    std::size_t topKCount = 0;

    if (offset < scoreSize)// if bigger is zero;
    {
        topKCount = scoreSize - offset;
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

    LOG(INFO) << "resultList size: " << resultList.size() 
              << ", topKCount: " << topKCount;

    for (unsigned int i = resultList.size() - topKCount; i < resultList.size(); ++i)
    {
        const ScoreDoc& scoreItem =  resultList[i];//need to ASC
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
        attrGroupParam.isAttrToken_ = zambeziManager_->isAttrTokenize();

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

    LOG(INFO) << "in zambezi ranking, total count: " << totalCount
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

    izenelib::util::ClockTimer timer;
    propSharedLockSet.insertSharedLock(categoryValueTable_);

    typedef std::vector<std::pair<faceted::PropValueTable::pvid_t, double> > TopCatIdsT;
    TopCatIdsT topCateIds;
    const std::size_t topNum = docIdList.size();
    std::set<faceted::PropValueTable::pvid_t> rootCateIds;

    for (std::size_t i = 0; i < topNum; ++i)
    {
        if (rootCateIds.size() >= kRootCateNum)
            break;

        category_id_t catId =
            categoryValueTable_->getFirstValueId(docIdList[i]);

        if (catId != 0)
        {
            bool is_exist = false;
            for (TopCatIdsT::const_iterator cit = topCateIds.begin();
                 cit != topCateIds.end(); ++cit)
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

                category_id_t rootId = categoryValueTable_->getRootValueId(catId);
                rootCateIds.insert(rootId);
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

    LOG(INFO) << "get top label num: "<< topLabels.size()
              << ", costs: " << timer.elapsed() << " seconds";
}

void ZambeziSearch::getAnalyzedQuery_(
    const std::string& rawQuery,
    izenelib::util::UString& analyzedQuery)
{
    b5m::ProductMatcher* matcher = b5m::ProductMatcherInstance::get();

    if (!matcher->IsOpen())
        return;

    typedef std::pair<izenelib::util::UString, double> TokenScore;
    typedef std::list<TokenScore> TokenScoreList;
    TokenScoreList majorTokens;
    TokenScoreList minorTokens;
    std::list<izenelib::util::UString> leftTokens;
    izenelib::util::UString queryUStr(rawQuery, izenelib::util::UString::UTF_8);

    matcher->GetSearchKeywords(queryUStr, majorTokens, minorTokens, leftTokens);

    for (TokenScoreList::const_iterator it = majorTokens.begin();
         it != majorTokens.end(); ++it)
    {
        const izenelib::util::UString& token = it->first;

        if (queryUStr.find(token) == izenelib::util::UString::npos)
            continue;

        analyzedQuery.append(token);
        analyzedQuery.push_back(kUCharSpace);
    }
}

void ZambeziSearch::normalizeScore_(
    std::vector<docid_t>& docids,
    std::vector<float>& scores,
    std::vector<float>& productScores,
    PropSharedLockSet &sharedLockSet)
{
    faceted::AttrTable* attTable = NULL;

    if (attrManager_)
    {
        attTable = &(attrManager_->getAttrTable());
        sharedLockSet.insertSharedLock(attTable);
    }
    float maxScore = 1;

    std::string propName = "itemcount";
    std::string propName_comment = "CommentCount";

    boost::shared_ptr<NumericPropertyTableBase> numericTable =
        numericTableBuilder_->createPropertyTable(propName);

    boost::shared_ptr<NumericPropertyTableBase> numericTable_comment =
        numericTableBuilder_->createPropertyTable(propName_comment);

    if (numericTable)
        sharedLockSet.insertSharedLock(numericTable.get());

    if (numericTable_comment)
        sharedLockSet.insertSharedLock(numericTable_comment.get());

    for (uint32_t i = 0; i < docids.size(); ++i)
    {
        uint32_t attr_size = 1;
        if (attTable)
        {
            faceted::AttrTable::ValueIdList attrvids;
            attTable->getValueIdList(docids[i], attrvids);
            attr_size = std::min(attrvids.size(), size_t(10));
        }

        int32_t itemcount = 1;
        if (numericTable)
        {
            numericTable->getInt32Value(docids[i], itemcount, false);
            attr_size += std::min(itemcount, 50);
        }

        if (numericTable_comment)
        {
            int32_t commentcount = 1;
            numericTable_comment->getInt32Value(docids[i], commentcount, false);
            if (itemcount != 0)
                attr_size += std::min(commentcount/itemcount, 100);
            else
                attr_size += std::min(commentcount, 100);

        }

        scores[i] = scores[i] * pow(attr_size, 0.3);
        if (scores[i] > maxScore)
            maxScore = scores[i];
    }

    for (unsigned int i = 0; i < scores.size(); ++i)
    {
        scores[i] = int(scores[i] / maxScore * 100) + productScores[i];
    }
}

bool ZambeziSearch::getZambeziAlgorithm(
     const int &algorithm,
     izenelib::ir::Zambezi::Algorithm& Algorithm)
{
    if (algorithm == 0)
    {
        Algorithm = izenelib::ir::Zambezi::SVS;
        return true;
    }
    else if (algorithm == 1)
    {
        Algorithm = izenelib::ir::Zambezi::WAND;
        return true;
    }
    else if (algorithm == 2)
    {
        Algorithm = izenelib::ir::Zambezi::MBWAND;
        return true;
    }
    else if (algorithm == 3)
    {
        Algorithm = izenelib::ir::Zambezi::BWAND_OR;
        return true;
    }
    else if (algorithm == 4)
    {
        Algorithm = izenelib::ir::Zambezi::BWAND_AND;
        return true;
    }
    return false;
}

}