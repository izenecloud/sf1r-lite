#include "ProductRankerTestFixture.h"
#include "../recommend-manager/test_util.h"
#include <mining-manager/product-ranker/ProductRankerFactory.h>
#include <mining-manager/product-ranker/ProductRankParam.h>
#include <mining-manager/product-ranker/ProductRanker.h>
#include <boost/test/unit_test.hpp>
#include <boost/scoped_ptr.hpp>
#include <algorithm> // sort

using namespace sf1r;

namespace
{
const std::string kTestDir = "test_product_ranker";
const std::string kMerchantPropName = "Source";

typedef std::vector<faceted::PropValueTable::pvid_t> PropValueIdList;
}

ProductRankerTestFixture::ProductRankerTestFixture()
    : merchantValueTable_(kTestDir, kMerchantPropName)
{
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

void ProductRankerTestFixture::setMultiDocMerchantId(const std::string& merchantList)
{
    PropValueIdList merchantIds;
    split_str_to_items(merchantList, merchantIds);

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
    split_str_to_items(merchantList, merchantIds);

    merchantValueTable_.appendPropIdList(merchantIds);
}

void ProductRankerTestFixture::rank()
{
    ProductRankerFactory rankerFactory(rankConfig_, &merchantValueTable_);
    const bool isRandomRank = (rankConfig_.scores[RANDOM_SCORE].weight != 0);
    ProductRankParam param(docIds_, topKScores_, isRandomRank);

    boost::scoped_ptr<ProductRanker> ranker(
        rankerFactory.createProductRanker(param));

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
