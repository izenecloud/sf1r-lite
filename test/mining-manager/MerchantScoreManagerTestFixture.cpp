#include "MerchantScoreManagerTestFixture.h"
#include <util/ustring/UString.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace
{
const char* TEST_DIR = "merchant_score_test";
const char* GROUP_DIR = "group";
const char* SCORE_FILE = "merchant_score.txt";

const char* MERCHANT_PROP = "Source";
const char* CATEGORY_PROP = "Category";

const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;

const float FLOAT_TOLERANCE = 0.00001;
}

namespace sf1r
{

MerchantScoreManagerTestFixture::MerchantScoreManagerTestFixture()
{
    bfs::path testDir = TEST_DIR;
    bfs::remove_all(testDir);
    bfs::create_directory(testDir);

    groupDir_ = (testDir / GROUP_DIR).string();
    bfs::create_directory(groupDir_);

    scoreFile_ = (testDir / SCORE_FILE).string();

    resetInstance();
}

void MerchantScoreManagerTestFixture::resetInstance()
{
    merchantScoreManager_.reset();
    merchantValueTable_.reset();
    categoryValueTable_.reset();

    merchantValueTable_.reset(new faceted::PropValueTable(groupDir_, MERCHANT_PROP));
    categoryValueTable_.reset(new faceted::PropValueTable(groupDir_, CATEGORY_PROP));
    merchantScoreManager_.reset(new MerchantScoreManager(merchantValueTable_.get(),
                                                         categoryValueTable_.get()));

    BOOST_CHECK(merchantValueTable_->open());
    BOOST_CHECK(categoryValueTable_->open());
    BOOST_CHECK(merchantScoreManager_->open(scoreFile_));
}

void MerchantScoreManagerTestFixture::checkEmptyScore()
{
    MerchantStrScoreMap strScoreMap;
    merchantScoreManager_->getAllStrScore(strScoreMap);
    BOOST_CHECK(strScoreMap.map.empty());

    checkNonExistingId_();
}

void MerchantScoreManagerTestFixture::setScore1()
{
    MerchantStrScoreMap strScoreMap;
    MerchantStrScoreMap::map_t& merchantMap = strScoreMap.map;

    {
        CategoryStrScore& category = merchantMap["京东"];
        category.generalScore = 6;
        category.categoryScoreMap["数码"] = 9;
        category.categoryScoreMap["家用电器"] = 8;
    }

    {
        CategoryStrScore& category = merchantMap["一号店"];
        category.generalScore = 7.7;
    }

    merchantScoreManager_->setScore(strScoreMap);
}

void MerchantScoreManagerTestFixture::checkScore1()
{
    checkAllScore1_();
    checkPartScore1_();
    checkIdScore1_();
}

void MerchantScoreManagerTestFixture::setScore2()
{
    MerchantStrScoreMap strScoreMap;
    MerchantStrScoreMap::map_t& merchantMap = strScoreMap.map;

    {
        CategoryStrScore& category = merchantMap["京东"];
        category.generalScore = 7;
        category.categoryScoreMap["数码"] = 9.8;
        category.categoryScoreMap["家用电器"] = 0;
        category.categoryScoreMap["服装服饰"] = 8;
    }

    {
        CategoryStrScore& category = merchantMap["一号店"];
        category.generalScore = 0;
        category.categoryScoreMap["食品"] = 8;
    }

    {
        CategoryStrScore& category = merchantMap["卓越亚马逊"];
        category.categoryScoreMap["图书"] = 9.5;
    }

    merchantScoreManager_->setScore(strScoreMap);
}

void MerchantScoreManagerTestFixture::checkScore2()
{
    checkAllScore2_();
    checkPartScore2_();
    checkIdScore2_();
}

merchant_id_t MerchantScoreManagerTestFixture::getMerchantId_(const std::string& merchant) const
{
    std::vector<izenelib::util::UString> path;
    izenelib::util::UString ustr(merchant, ENCODING_TYPE);
    path.push_back(ustr);

    return merchantValueTable_->propValueId(path);
}

category_id_t MerchantScoreManagerTestFixture::getCategoryId_(const std::string& category) const
{
    std::vector<izenelib::util::UString> path;
    izenelib::util::UString ustr(category, ENCODING_TYPE);
    path.push_back(ustr);

    return categoryValueTable_->propValueId(path);
}

void MerchantScoreManagerTestFixture::checkNonExistingId_()
{
    merchant_id_t merchantId = getMerchantId_("不存在");
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, 0), 0);

    category_id_t categoryId = getCategoryId_("不存在");
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, categoryId), 0);
}

void MerchantScoreManagerTestFixture::checkAllScore1_()
{
    MerchantStrScoreMap strScoreMap;
    merchantScoreManager_->getAllStrScore(strScoreMap);

    MerchantStrScoreMap::map_t& merchantMap = strScoreMap.map;
    BOOST_CHECK_EQUAL(merchantMap.size(), 2U);

    {
        CategoryStrScore& category = merchantMap["京东"];
        BOOST_CHECK_EQUAL(category.generalScore, 6);
        BOOST_CHECK_EQUAL(category.categoryScoreMap.size(), 2U);
        BOOST_CHECK_EQUAL(category.categoryScoreMap["数码"], 9);
        BOOST_CHECK_EQUAL(category.categoryScoreMap["家用电器"], 8);
    }

    {
        CategoryStrScore& category = merchantMap["一号店"];
        BOOST_CHECK_CLOSE(category.generalScore, 7.7, FLOAT_TOLERANCE);
        BOOST_CHECK_EQUAL(category.categoryScoreMap.size(), 0U);
    }
}

void MerchantScoreManagerTestFixture::checkPartScore1_()
{
    MerchantStrScoreMap strScoreMap;
    std::vector<std::string> merchantNames;
    merchantNames.push_back("京东");
    merchantNames.push_back("易讯");

    merchantScoreManager_->getStrScore(merchantNames, strScoreMap);

    MerchantStrScoreMap::map_t& merchantMap = strScoreMap.map;
    BOOST_CHECK_EQUAL(merchantMap.size(), 2U);

    {
        CategoryStrScore& category = merchantMap["京东"];
        BOOST_CHECK_EQUAL(category.generalScore, 6);
        BOOST_CHECK_EQUAL(category.categoryScoreMap.size(), 2U);
        BOOST_CHECK_EQUAL(category.categoryScoreMap["数码"], 9);
        BOOST_CHECK_EQUAL(category.categoryScoreMap["家用电器"], 8);
    }

    {
        CategoryStrScore& category = merchantMap["易讯"];
        BOOST_CHECK_EQUAL(category.generalScore, 0);
        BOOST_CHECK_EQUAL(category.categoryScoreMap.size(), 0U);
    }
}

void MerchantScoreManagerTestFixture::checkIdScore1_()
{
    checkNonExistingId_();

    merchant_id_t merchantId = getMerchantId_("京东");

    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, 0),
                                                        6);
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, getCategoryId_("数码")),
                                                        9);
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, getCategoryId_("家用电器")),
                                                        8);
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, getCategoryId_("食品")),
                                                        6);

    merchantId = getMerchantId_("一号店");

    BOOST_CHECK_CLOSE(merchantScoreManager_->getIdScore(merchantId, 0),
                                                        7.7, FLOAT_TOLERANCE);
    BOOST_CHECK_CLOSE(merchantScoreManager_->getIdScore(merchantId, getCategoryId_("食品")),
                                                        7.7, FLOAT_TOLERANCE);
}

void MerchantScoreManagerTestFixture::checkAllScore2_()
{
    MerchantStrScoreMap strScoreMap;
    merchantScoreManager_->getAllStrScore(strScoreMap);

    MerchantStrScoreMap::map_t& merchantMap = strScoreMap.map;
    BOOST_CHECK_EQUAL(merchantMap.size(), 3U);

    {
        CategoryStrScore& category = merchantMap["京东"];
        BOOST_CHECK_EQUAL(category.generalScore, 7);
        BOOST_CHECK_EQUAL(category.categoryScoreMap.size(), 3U);
        BOOST_CHECK_CLOSE(category.categoryScoreMap["数码"], 9.8, FLOAT_TOLERANCE);
        BOOST_CHECK_EQUAL(category.categoryScoreMap["家用电器"], 0);
        BOOST_CHECK_EQUAL(category.categoryScoreMap["服装服饰"], 8);
    }

    {
        CategoryStrScore& category = merchantMap["一号店"];
        BOOST_CHECK_EQUAL(category.generalScore, 0);
        BOOST_CHECK_EQUAL(category.categoryScoreMap.size(), 1U);
        BOOST_CHECK_EQUAL(category.categoryScoreMap["食品"], 8);
    }

    {
        CategoryStrScore& category = merchantMap["卓越亚马逊"];
        BOOST_CHECK_EQUAL(category.generalScore, 0);
        BOOST_CHECK_EQUAL(category.categoryScoreMap.size(), 1U);
        BOOST_CHECK_EQUAL(category.categoryScoreMap["图书"], 9.5);
    }
}

void MerchantScoreManagerTestFixture::checkPartScore2_()
{
    MerchantStrScoreMap strScoreMap;
    std::vector<std::string> merchantNames;
    merchantNames.push_back("京东");
    merchantNames.push_back("易讯");

    merchantScoreManager_->getStrScore(merchantNames, strScoreMap);

    MerchantStrScoreMap::map_t& merchantMap = strScoreMap.map;
    BOOST_CHECK_EQUAL(merchantMap.size(), 2U);

    {
        CategoryStrScore& category = merchantMap["京东"];
        BOOST_CHECK_EQUAL(category.generalScore, 7);
        BOOST_CHECK_EQUAL(category.categoryScoreMap.size(), 3U);
        BOOST_CHECK_CLOSE(category.categoryScoreMap["数码"], 9.8, FLOAT_TOLERANCE);
        BOOST_CHECK_EQUAL(category.categoryScoreMap["家用电器"], 0);
        BOOST_CHECK_EQUAL(category.categoryScoreMap["服装服饰"], 8);
    }

    {
        CategoryStrScore& category = merchantMap["易讯"];
        BOOST_CHECK_EQUAL(category.generalScore, 0);
        BOOST_CHECK_EQUAL(category.categoryScoreMap.size(), 0U);
    }
}

void MerchantScoreManagerTestFixture::checkIdScore2_()
{
    checkNonExistingId_();

    merchant_id_t merchantId = getMerchantId_("京东");

    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, 0),
                                                        7);
    BOOST_CHECK_CLOSE(merchantScoreManager_->getIdScore(merchantId, getCategoryId_("数码")),
                                                        9.8, FLOAT_TOLERANCE);
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, getCategoryId_("家用电器")),
                                                        0);
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, getCategoryId_("服装服饰")),
                                                        8);
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, getCategoryId_("食品")),
                                                        7);

    merchantId = getMerchantId_("一号店");

    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, 0),
                                                        0);
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, getCategoryId_("食品")),
                                                        8);
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, getCategoryId_("数码")),
                                                        0);

    merchantId = getMerchantId_("卓越亚马逊");

    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, 0),
                                                        0);
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, getCategoryId_("图书")),
                                                        9.5);
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, getCategoryId_("数码")),
                                                        0);
}

} // namespace sf1r
