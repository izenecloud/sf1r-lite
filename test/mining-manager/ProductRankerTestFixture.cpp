#include "ProductRankerTestFixture.h"
#include "../recommend-manager/test_util.h"
#include <mining-manager/product-ranker/ProductRankerFactory.h>
#include <mining-manager/product-ranker/ProductRankParam.h>
#include <mining-manager/product-ranker/ProductRanker.h>
#include <common/NumericPropertyTable.h>
#include <util/ustring/UString.h>
#include <boost/test/unit_test.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <algorithm> // sort

using namespace sf1r;
namespace bfs = boost::filesystem;

namespace
{
const std::string kTestDir = "test_product_ranker";
const std::string kMerchantPropName = "Source";
const std::string kQueryStr = "iphone";
const izenelib::util::UString::EncodingType ENCODING_TYPE =
    izenelib::util::UString::UTF_8;
}

ProductRankerTestFixture::ProductRankerTestFixture()
    : merchantValueTable_(kTestDir, kMerchantPropName)
    , merchantScoreManager_(&merchantValueTable_, NULL)
    , offerItemCountTable_(new NumericPropertyTable<int32_t>(INT32_PROPERTY_TYPE))
{
    bfs::remove_all(kTestDir);
    bfs::create_directories(kTestDir);

    rankConfig_.scores[CUSTOM_SCORE].weight = 10;
    rankConfig_.scores[CATEGORY_SCORE].weight = 1;
}

void ProductRankerTestFixture::configRandomScore()
{
    rankConfig_.scores[RANDOM_SCORE].weight = 1;
}

void ProductRankerTestFixture::setDocId(const std::string& docIdList)
{
    docIds_.clear();
    split_str_to_items(docIdList, docIds_);
}

void ProductRankerTestFixture::setTopKScore(const std::string& scoreList)
{
    topKScores_.clear();
    split_str_to_items(scoreList, topKScores_);
}

void ProductRankerTestFixture::setOfferItemCount(const std::string& offerCountList)
{
    std::vector<int32_t> offerCounts;
    split_str_to_items(offerCountList, offerCounts);

    docid_t docId = 1;
    for (std::vector<int32_t>::const_iterator it = offerCounts.begin();
         it != offerCounts.end(); ++it)
    {
        offerItemCountTable_->setInt32Value(docId++, *it);
    }
}

void ProductRankerTestFixture::setMultiDocMerchantId(const std::string& merchantList)
{
    PropValueIdList merchantIds;
    convertMerchantId_(merchantList, merchantIds);

    for (PropValueIdList::const_iterator it = merchantIds.begin();
         it != merchantIds.end(); ++it)
    {
        PropValueIdList singleIdList;
        singleIdList.push_back(*it);
        merchantValueTable_.appendPropIdList(singleIdList);
    }
}

void ProductRankerTestFixture::setSingleDocMerchantId(const std::string& merchantList)
{
    PropValueIdList merchantIds;
    convertMerchantId_(merchantList, merchantIds);

    merchantValueTable_.appendPropIdList(merchantIds);
}

void ProductRankerTestFixture::convertMerchantId_(
    const std::string& merchantList,
    PropValueIdList& idList)
{
    typedef std::vector<std::string> StrList;
    StrList strList;
    split_str_to_items(merchantList, strList);

    for (StrList::const_iterator it = strList.begin(); it != strList.end(); ++it)
    {
        merchant_id_t merchantId = 0;
        std::vector<izenelib::util::UString> path;
        path.push_back(izenelib::util::UString(*it, ENCODING_TYPE));

        merchantId = merchantValueTable_.insertPropValueId(path);
        idList.push_back(merchantId);
    }
}

void ProductRankerTestFixture::setMerchantScore(const std::string& merchantScoreList)
{
    std::vector<score_t> scoreList;
    split_str_to_items(merchantScoreList, scoreList);

    MerchantStrScoreMap merchantScoreMap;
    for (std::size_t i = 0; i < scoreList.size(); ++i)
    {
        category_id_t merchantId = i + 1;
        std::string merchantStr = boost::lexical_cast<std::string>(merchantId);
        merchantScoreMap.map[merchantStr].generalScore = scoreList[i];
    }

    merchantScoreManager_.setScore(merchantScoreMap);
}

void ProductRankerTestFixture::rank()
{
    const bool isRandomRank = (rankConfig_.scores[RANDOM_SCORE].weight != 0);
    ProductRankParam param(docIds_,
                           topKScores_,
                           isRandomRank,
                           kQueryStr,
                           SearchingMode::DefaultSearchingMode);
    ProductRankerFactory rankerFactory(rankConfig_,
                                       NULL,
                                       offerItemCountTable_,
                                       &merchantValueTable_,
                                       &merchantScoreManager_);

    boost::scoped_ptr<ProductRanker> ranker(
        rankerFactory.createProductRanker(param));

    BOOST_REQUIRE(ranker);
    ranker->rank();
}

void ProductRankerTestFixture::checkDocId(const std::string& goldDocIdList)
{
    vector<docid_t> goldDocIds;
    split_str_to_items(goldDocIdList, goldDocIds);

    BOOST_CHECK_EQUAL_COLLECTIONS(docIds_.begin(), docIds_.end(),
                                  goldDocIds.begin(), goldDocIds.end());
}

void ProductRankerTestFixture::checkTopKScore(const std::string& goldScoreList)
{
    std::vector<score_t> goldScores;
    split_str_to_items(goldScoreList, goldScores);

    BOOST_CHECK_EQUAL_COLLECTIONS(topKScores_.begin(), topKScores_.end(),
                                  goldScores.begin(), goldScores.end());
}

void ProductRankerTestFixture::checkDocId(
    const std::string& goldDocIdList,
    int equalNum)
{
    vector<docid_t> goldDocIds;
    split_str_to_items(goldDocIdList, goldDocIds);

    BOOST_CHECK_EQUAL_COLLECTIONS(docIds_.begin(),
                                  docIds_.begin() + equalNum,
                                  goldDocIds.begin(),
                                  goldDocIds.begin() + equalNum);

    std::sort(docIds_.begin(), docIds_.end());
    std::sort(goldDocIds.begin(), goldDocIds.end());

    BOOST_CHECK_EQUAL_COLLECTIONS(docIds_.begin(), docIds_.end(),
                                  goldDocIds.begin(), goldDocIds.end());
}

void ProductRankerTestFixture::checkTopKScore(
    const std::string& goldScoreList,
    int equalNum)
{
    std::vector<score_t> goldScores;
    split_str_to_items(goldScoreList, goldScores);

    BOOST_CHECK_EQUAL_COLLECTIONS(topKScores_.begin(),
                                  topKScores_.begin() + equalNum,
                                  goldScores.begin(),
                                  goldScores.begin() + equalNum);

    std::sort(topKScores_.begin(), topKScores_.end());
    std::sort(goldScores.begin(), goldScores.end());

    BOOST_CHECK_EQUAL_COLLECTIONS(topKScores_.begin(), topKScores_.end(),
                                  goldScores.begin(), goldScores.end());
}
