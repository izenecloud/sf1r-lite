#include "MerchantScoreManagerTestFixture.h"
#include "../recommend-manager/test_util.h"
#include <util/ustring/UString.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace
{
const char* TEST_DIR = "merchant_score_test";
const char* GROUP_DIR = "group";
const char* SCORE_FILE = "merchant_score.bin";

const char* MERCHANT_PROP = "Source";
const char* CATEGORY_PROP = "Category";

const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;

const float FLOAT_TOLERANCE = 0.00001;
}

namespace sf1r
{

/**
 * @param categoryStr a string containing a category path,
 * from root node to leaf node, each splitted by space.
 * For example, "node1 node2 node3".
 */
score_t& getCategoryScore(
    CategoryStrScore& categoryStrScore,
    const std::string& categoryStr)
{
    CategoryStrPath categoryPath;
    split_str_to_items(categoryStr, categoryPath);

    return categoryStrScore.categoryScoreMap[categoryPath];
}

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
        getCategoryScore(category, "手机数码 手机通讯 手机") = 9;
        getCategoryScore(category, "家用电器") = 8;
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
        category.generalScore = 5;
        getCategoryScore(category, "手机数码 手机通讯 手机") = 9.8;
        getCategoryScore(category, "手机数码 手机通讯") = 8.8;
        getCategoryScore(category, "手机数码") = 7.8;
        getCategoryScore(category, "服装服饰 男装") = 6;
    }

    {
        CategoryStrScore& category = merchantMap["一号店"];
        category.generalScore = 0;
        getCategoryScore(category, "食品") = 8;
    }

    {
        CategoryStrScore& category = merchantMap["卓越亚马逊"];
        getCategoryScore(category, "图书") = 9.5;
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
    CategoryStrPath categoryPath;
    split_str_to_items(category, categoryPath);

    std::vector<izenelib::util::UString> ustrPath;
    for (CategoryStrPath::const_iterator it = categoryPath.begin();
         it != categoryPath.end(); ++it)
    {
        ustrPath.push_back(izenelib::util::UString(*it, ENCODING_TYPE));
    }

    return categoryValueTable_->propValueId(ustrPath);
}

category_id_t MerchantScoreManagerTestFixture::insertCategoryId_(const std::string& category) const
{
    CategoryStrPath categoryPath;
    split_str_to_items(category, categoryPath);

    std::vector<izenelib::util::UString> ustrPath;
    for (CategoryStrPath::const_iterator it = categoryPath.begin();
         it != categoryPath.end(); ++it)
    {
        ustrPath.push_back(izenelib::util::UString(*it, ENCODING_TYPE));
    }

    return categoryValueTable_->insertPropValueId(ustrPath);
}

void MerchantScoreManagerTestFixture::checkNonExistingId_()
{
    merchant_id_t merchantId = getMerchantId_("不存在");
    std::vector<category_id_t> categoryParentIds;
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, categoryParentIds),
                      0);

    category_id_t categoryId = getCategoryId_("手机数码");
    categoryParentIds.push_back(categoryId);
    BOOST_CHECK_EQUAL(merchantScoreManager_->getIdScore(merchantId, categoryParentIds),
                      0);
}

void MerchantScoreManagerTestFixture::checkIdScoreEqual_(
    const std::string& merchant,
    const std::string& category,
    score_t gold)
{
    BOOST_TEST_MESSAGE("checkIdScoreEqual_, merchant: " << merchant <<
                       ", category: " << category);

    merchant_id_t merchantId = getMerchantId_(merchant);
    category_id_t categoryId = getCategoryId_(category);
    std::vector<category_id_t> categoryParentIds;
    categoryValueTable_->getParentIds(categoryId, categoryParentIds);
    score_t actual = merchantScoreManager_->getIdScore(merchantId, categoryParentIds);

    BOOST_CHECK_EQUAL(actual, gold);
}

void MerchantScoreManagerTestFixture::checkIdScoreClose_(
    const std::string& merchant,
    const std::string& category,
    score_t gold)
{
    BOOST_TEST_MESSAGE("checkIdScoreClose_, merchant: " << merchant <<
                       ", category: " << category);

    merchant_id_t merchantId = getMerchantId_(merchant);
    category_id_t categoryId = getCategoryId_(category);
    std::vector<category_id_t> categoryParentIds;
    categoryValueTable_->getParentIds(categoryId, categoryParentIds);
    score_t actual = merchantScoreManager_->getIdScore(merchantId, categoryParentIds);

    BOOST_CHECK_CLOSE(actual, gold, FLOAT_TOLERANCE);
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
        BOOST_CHECK_EQUAL(getCategoryScore(category, "手机数码 手机通讯 手机"), 9);
        BOOST_CHECK_EQUAL(getCategoryScore(category, "家用电器"), 8);
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
        BOOST_CHECK_EQUAL(getCategoryScore(category, "手机数码 手机通讯 手机"), 9);
        BOOST_CHECK_EQUAL(getCategoryScore(category, "家用电器"), 8);
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

    std::string merchant = "京东";
    checkIdScoreEqual_(merchant, "", 6);
    checkIdScoreEqual_(merchant, "手机数码 手机通讯 手机", 9);
    checkIdScoreEqual_(merchant, "手机数码 手机通讯", 6);
    checkIdScoreEqual_(merchant, "手机数码", 6);
    checkIdScoreEqual_(merchant, "家用电器", 8);
    checkIdScoreEqual_(merchant, "食品", 6);

    std::string subNode("手机数码 手机通讯 手机 iPad");
    insertCategoryId_(subNode);
    checkIdScoreEqual_(merchant, subNode, 9);

    merchant = "一号店";
    checkIdScoreClose_(merchant, "", 7.7);
    checkIdScoreClose_(merchant, "食品", 7.7);
}

void MerchantScoreManagerTestFixture::checkAllScore2_()
{
    MerchantStrScoreMap strScoreMap;
    merchantScoreManager_->getAllStrScore(strScoreMap);

    MerchantStrScoreMap::map_t& merchantMap = strScoreMap.map;
    BOOST_CHECK_EQUAL(merchantMap.size(), 3U);

    {
        CategoryStrScore& category = merchantMap["京东"];
        BOOST_CHECK_EQUAL(category.generalScore, 5);
        BOOST_CHECK_EQUAL(category.categoryScoreMap.size(), 4U);
        BOOST_CHECK_CLOSE(getCategoryScore(category, "手机数码 手机通讯 手机"),
                          9.8, FLOAT_TOLERANCE);
        BOOST_CHECK_CLOSE(getCategoryScore(category, "手机数码 手机通讯"),
                          8.8, FLOAT_TOLERANCE);
        BOOST_CHECK_CLOSE(getCategoryScore(category, "手机数码"),
                          7.8, FLOAT_TOLERANCE);
        BOOST_CHECK_EQUAL(getCategoryScore(category, "服装服饰 男装"), 6);
    }

    {
        CategoryStrScore& category = merchantMap["一号店"];
        BOOST_CHECK_EQUAL(category.generalScore, 0);
        BOOST_CHECK_EQUAL(category.categoryScoreMap.size(), 1U);
        BOOST_CHECK_EQUAL(getCategoryScore(category, "食品"), 8);
    }

    {
        CategoryStrScore& category = merchantMap["卓越亚马逊"];
        BOOST_CHECK_EQUAL(category.generalScore, 0);
        BOOST_CHECK_EQUAL(category.categoryScoreMap.size(), 1U);
        BOOST_CHECK_EQUAL(getCategoryScore(category, "图书"), 9.5);
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
        BOOST_CHECK_EQUAL(category.generalScore, 5);
        BOOST_CHECK_EQUAL(category.categoryScoreMap.size(), 4U);
        BOOST_CHECK_CLOSE(getCategoryScore(category, "手机数码 手机通讯 手机"),
                          9.8, FLOAT_TOLERANCE);
        BOOST_CHECK_CLOSE(getCategoryScore(category, "手机数码 手机通讯"),
                          8.8, FLOAT_TOLERANCE);
        BOOST_CHECK_CLOSE(getCategoryScore(category, "手机数码"),
                          7.8, FLOAT_TOLERANCE);
        BOOST_CHECK_EQUAL(getCategoryScore(category, "服装服饰 男装"), 6);
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

    std::string merchant = "京东";
    checkIdScoreEqual_(merchant, "", 5);
    checkIdScoreClose_(merchant, "手机数码 手机通讯 手机", 9.8);
    checkIdScoreClose_(merchant, "手机数码 手机通讯", 8.8);
    checkIdScoreClose_(merchant, "手机数码", 7.8);
    checkIdScoreEqual_(merchant, "家用电器", 5);
    checkIdScoreEqual_(merchant, "服装服饰 男装", 6);
    checkIdScoreEqual_(merchant, "食品", 5);

    std::string subNode("手机数码 手机通讯 手机 iPhone");
    insertCategoryId_(subNode);
    checkIdScoreClose_(merchant, subNode, 9.8);

    merchant = "一号店";
    checkIdScoreEqual_(merchant, "", 0);
    checkIdScoreEqual_(merchant, "食品", 8);
    checkIdScoreEqual_(merchant, "数码", 0);

    merchant = "卓越亚马逊";
    checkIdScoreEqual_(merchant, "", 0);
    checkIdScoreEqual_(merchant, "图书", 9.5);
    checkIdScoreEqual_(merchant, "数码", 0);
}

} // namespace sf1r
