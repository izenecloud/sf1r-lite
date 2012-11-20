#include "ProductScoreManagerTestFixture.h"
#include "RandomScorerFactory.h"
#include <mining-manager/product-score-manager/ProductScoreManager.h>
#include <mining-manager/product-scorer/ProductScorer.h>
#include <document-manager/DocumentManager.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>
#include <ctime>
#include <glog/logging.h>

using namespace sf1r;
namespace bfs = boost::filesystem;

namespace
{
const char* TEST_DIR_STR = "product_score_test";
const char* TEST_COLLECTION_NAME = "example";

const izenelib::util::UString::EncodingType ENCODING_TYPE =
    izenelib::util::UString::UTF_8;

const ProductScoreType TEST_SCORE_TYPE = POPULARITY_SCORE;
}

ProductScoreManagerTestFixture::ProductScoreManagerTestFixture()
    : offlineScorerFactory_(NULL)
    , productScoreManager_(NULL)
    , documentManager_(NULL)
    , lastDocId_(0)
    , goldScorerFactory_(NULL)
    , goldScorer_(NULL)
{
    boost::filesystem::remove_all(TEST_DIR_STR);

    scoreDirPath_ = (bfs::path(TEST_DIR_STR) / "score").string();
    bfs::path dmPath(bfs::path(TEST_DIR_STR) / "dm/");
    bfs::create_directories(dmPath);

    initConfig_();

    documentManager_ = new DocumentManager(dmPath.string(),
                                           schema_,
                                           ENCODING_TYPE,
                                           2000);

    unsigned int seed = time(NULL);
    offlineScorerFactory_ = new RandomScorerFactory(seed);
    goldScorerFactory_ = new RandomScorerFactory(seed);

    const ProductScoreConfig& scoreConfig = rankConfig_.scores[TEST_SCORE_TYPE];
    goldScorer_ = goldScorerFactory_->createScorer(scoreConfig);

    resetScoreManager();
}

void ProductScoreManagerTestFixture::initConfig_()
{
    PropertyConfigBase config;
    config.propertyName_ = "DOCID";
    config.propertyType_ = STRING_PROPERTY_TYPE;
    schema_.insert(config);

    rankConfig_.isEnable = true;
    ProductScoreConfig& scoreConfig = rankConfig_.scores[TEST_SCORE_TYPE];
    scoreConfig.weight = 1;
}

ProductScoreManagerTestFixture::~ProductScoreManagerTestFixture()
{
    delete documentManager_;
    delete productScoreManager_;
    delete offlineScorerFactory_;
    delete goldScorer_;
    delete goldScorerFactory_;
}

void ProductScoreManagerTestFixture::resetScoreManager()
{
    delete productScoreManager_;

    productScoreManager_ = new ProductScoreManager(rankConfig_,
                                                   bundleParam_,
                                                   *offlineScorerFactory_,
                                                   *documentManager_,
                                                   TEST_COLLECTION_NAME,
                                                   scoreDirPath_);

    BOOST_CHECK(productScoreManager_->open());
}

void ProductScoreManagerTestFixture::buildScore(int docNum)
{
    insertDocument(docNum);

    buildCollection();
}

void ProductScoreManagerTestFixture::insertDocument(int docNum)
{
    for (int i = 0; i < docNum; ++i)
    {
        ++lastDocId_;

        Document document(lastDocId_);
        std::string docIdStr = boost::lexical_cast<std::string>(lastDocId_);
        izenelib::util::UString property(docIdStr, ENCODING_TYPE);
        document.property("DOCID") = property;

        BOOST_CHECK(documentManager_->insertDocument(document));
    }
}

void ProductScoreManagerTestFixture::buildCollection()
{
    BOOST_CHECK(productScoreManager_->buildCollection());

    goldScores_.resize(lastDocId_ + 1);
    for (docid_t docId = 1; docId <= lastDocId_; ++docId)
    {
        goldScores_[docId] = goldScorer_->score(docId);
    }
}

void ProductScoreManagerTestFixture::checkScore()
{
    boost::scoped_ptr<ProductScorer> productScorer(
        productScoreManager_->createProductScorer(TEST_SCORE_TYPE));

    BOOST_REQUIRE(productScorer);

    for (docid_t docId = 1; docId <= lastDocId_; ++docId)
    {
        BOOST_CHECK_EQUAL(productScorer->score(docId), goldScores_[docId]);
    }

    BOOST_CHECK_EQUAL(productScorer->score(0), 0);
    BOOST_CHECK_EQUAL(productScorer->score(lastDocId_+1), 0);
}

void ProductScoreManagerTestFixture::readScore(int docNum)
{
    LOG(INFO) << "start readScore(), docNum: " << docNum;

    boost::scoped_ptr<ProductScorer> productScorer(
        productScoreManager_->createProductScorer(TEST_SCORE_TYPE));

    score_t sum = 0;
    for (int docId = 1; docId <= docNum; ++docId)
    {
        sum += productScorer->score(docId);
    }

    LOG(INFO) << "end readScore(), score sum: " << sum;
}
