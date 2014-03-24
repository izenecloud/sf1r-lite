#include "ZambeziSearch.h"
#include "ZambeziFilter.h"
#include "ZambeziScoreNormalizer.h"
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
#include <util/fmath/fmath.hpp>
#include <mining-manager/group-manager/GroupFilterBuilder.h>
#include <mining-manager/group-manager/GroupFilter.h>
#include <mining-manager/product-scorer/ProductScorer.h>
#include <mining-manager/util/convert_ustr.h>
#include <b5m-manager/product_matcher.h>
#include <ir/index_manager/utility/Bitset.h>
#include <util/ClockTimer.h>
#include <glog/logging.h>
#include <iostream>
#include <set>
#include <math.h>
#include <algorithm>
#include <boost/scoped_ptr.hpp>

namespace sf1r
{

namespace
{
const std::size_t kAttrTopDocNum = 200;
const std::size_t kZambeziTopKNum = 5e5;

const std::string kTopLabelPropName = "Category";
const size_t kRootCateNum = 10;

const std::string kMerchantPropName = "Source";
const izenelib::util::UString::EncodingType kEncodeType =
    izenelib::util::UString::UTF_8;
const izenelib::util::UString kAttrExcludeMerchant =
    izenelib::util::UString("淘宝网", kEncodeType);

const izenelib::util::UString::CharT kUCharSpace = ' ';
}

bool DocLess(const ScoreDoc& o1, const ScoreDoc& o2)
{
    if (o1.score == o2.score)
        return (o1.docId > o2.docId);

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
    , merchantValueTable_(NULL)
{
}

void ZambeziSearch::setMiningManager(
    const boost::shared_ptr<MiningManager>& miningManager)
{
    groupFilterBuilder_ = miningManager->GetGroupFilterBuilder();
    categoryValueTable_ = miningManager->GetPropValueTable(kTopLabelPropName);
    merchantValueTable_ = miningManager->GetPropValueTable(kMerchantPropName);
    zambeziScoreNormalizer_.reset(new ZambeziScoreNormalizer(*miningManager));
    attrManager_= miningManager->GetAttributeManager();
    numericTableBuilder_ = miningManager->GetNumericTableBuilder();
}

void ZambeziSearch::normalizeTopDocs_(
    const boost::scoped_ptr<ProductScorer>& productScorer,
    boost::scoped_ptr<HitQueue>& scoreItemQueue,
    std::vector<ScoreDoc>& resultList)
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
    // if (zambeziManager_->isAttrTokenize())
    //     normalizeScore_(topDocids, topRelevanceScores, topProductScores, sharedLockSet);
    if (zambeziManager_->isAttrTokenize())
        zambeziScoreNormalizer_->normalizeScore(topDocids,
                                            topProductScores,
                                            topRelevanceScores);

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

    ConditionsNode& filterTree =
        actionOperation.actionItem_.filterTree_;

    boost::shared_ptr<InvertedIndexManager::FilterBitmapT> filterBitmap;
    boost::shared_ptr<izenelib::ir::indexmanager::Bitset> filterBitset;

    if (!filterTree.empty())
    {
        queryBuilder_.prepare_filter(filterTree, filterBitmap);
        filterBitset.reset(new izenelib::ir::indexmanager::Bitset);
        filterBitset->decompress(*filterBitmap);
    }
    //Query Analyzer
    getAnalyzedQuery_(query, searchResult.analyzedQuery_);

    std::vector<std::pair<std::string, int> > tokenList;

    if (zambeziManager_->isAttrTokenize())
        AttrTokenizeWrapper::get()->attr_tokenize(query, tokenList); // kevin'dict
    else
        zambeziManager_->getTokenizer()->getTokenResults(query, tokenList);

    if (tokenList.empty())
        return false;
    
    ZambeziFilter filter(documentManager_, groupFilter, filterBitset);

    zambeziManager_->search(algorithm, tokenList, search_in_properties,
                            &filter, kZambeziTopKNum, candidates, scores);

    if (candidates.empty() && zambeziManager_->isAttrTokenize())
    {
        std::vector<std::pair<std::string, int> > subTokenList;
        AttrTokenizeWrapper::get()->attr_tokenize(query, subTokenList, true); // kevin'dict

        zambeziManager_->search(algorithm, subTokenList, search_in_properties,
                                &filter, kZambeziTopKNum, candidates, scores);
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

    //normalize relevance scores
    if (zambeziManager_->isAttrTokenize() && tokenList.size() > 1)
    {
        float normalizerScore = 0;
        for (std::vector<std::pair<std::string, int> >::const_iterator it = tokenList.begin();
                it != tokenList.end(); ++it)
        {
            normalizerScore += it->second;
        }
        for (std::vector<float>::iterator it = scores.begin(); it != scores.end(); ++it)
        {
            *it /= normalizerScore;
        }
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

    if (groupFilterBuilder_)
    {
        groupFilter.reset(
            groupFilterBuilder_->createFilter(groupParam, propSharedLockSet));
    }

    // reset relevance score
    const std::size_t candNum = candidates.size();
    std::size_t totalCount = 0;

    {
        for (size_t i = 0; i < candNum; ++i)
        {
            docid_t docId = candidates[i];

            if (groupFilter && !groupFilter->test(docId))
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
                        resultList);
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
    /// end

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

    for (unsigned int i = 0; i < topKCount; ++i)
    {
        const ScoreDoc& scoreItem =  resultList[i + offset];
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

    if (originIsAttrGroup)
    {
        getTopAttrs_(docIdList, groupParam, propSharedLockSet,
                     searchResult.attrRep_);
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

    typedef std::vector<std::pair<faceted::PropValueTable::pvid_t, faceted::GroupPathScoreInfo> > TopCatIdsT;
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
                topCateIds.push_back(std::make_pair(catId, faceted::GroupPathScoreInfo(rankScoreList[i], docIdList[i])));

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

void ZambeziSearch::getTopAttrs_(
    const std::vector<unsigned int>& docIdList,
    faceted::GroupParam& groupParam,
    PropSharedLockSet& propSharedLockSet,
    faceted::OntologyRep& attrRep)
{
    if (!groupFilterBuilder_)
        return;

    izenelib::util::ClockTimer timer;

    faceted::GroupParam attrGroupParam;
    attrGroupParam.isAttrGroup_ = groupParam.isAttrGroup_ = true;
    attrGroupParam.attrGroupNum_ = groupParam.attrGroupNum_;
    attrGroupParam.searchMode_ = groupParam.searchMode_;

    boost::scoped_ptr<faceted::GroupFilter> attrGroupFilter(
        groupFilterBuilder_->createFilter(attrGroupParam, propSharedLockSet));

    if (!attrGroupFilter)
        return;

    faceted::PropValueTable::pvid_t excludeMerchantId = 0;
    if (merchantValueTable_)
    {
        std::vector<izenelib::util::UString> path;
        path.push_back(kAttrExcludeMerchant);

        propSharedLockSet.insertSharedLock(merchantValueTable_);
        excludeMerchantId = merchantValueTable_->propValueId(path, false);
    }

    size_t testNum = 0;
    for (size_t i = 0; i < docIdList.size() && testNum < kAttrTopDocNum; ++i)
    {
        docid_t docId = docIdList[i];

        if (excludeMerchantId &&
            merchantValueTable_->testDoc(docId, excludeMerchantId))
            continue;

        attrGroupFilter->test(docId);
        ++testNum;
    }

    faceted::GroupRep tempGroupRep;
    attrGroupFilter->getGroupRep(tempGroupRep, attrRep);

    LOG(INFO) << "attrGroupFilter costs :" << timer.elapsed() << " seconds";
}

void ZambeziSearch::getAnalyzedQuery_(
    const std::string& rawQuery,
    izenelib::util::UString& analyzedQuery)
{
    //TODO 2014-03-24
    typedef std::pair<izenelib::util::UString, double> TokenScore;
    typedef std::list<TokenScore> TokenScoreList;
    TokenScoreList majorTokens;
    TokenScoreList minorTokens;
    std::list<izenelib::util::UString> leftTokens;
    izenelib::util::UString queryUStr(rawQuery, izenelib::util::UString::UTF_8);

    //matcher->GetSearchKeywords(queryUStr, majorTokens, minorTokens, leftTokens);

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
