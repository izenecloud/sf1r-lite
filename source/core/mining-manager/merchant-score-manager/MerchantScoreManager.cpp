#include "MerchantScoreManager.h"
#include "MerchantScoreParser.h"
#include "MerchantScoreRenderer.h"
#include "../faceted-submanager/prop_value_table.h"
#include "../MiningException.hpp"
#include <util/ustring/UString.h>

#include <glog/logging.h>

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
}

namespace sf1r
{

MerchantScoreManager::MerchantScoreManager(
    faceted::PropValueTable* merchantValueTable,
    faceted::PropValueTable* categoryValueTable
)
    : merchantValueTable_(merchantValueTable)
    , categoryValueTable_(categoryValueTable)
{
}

MerchantScoreManager::~MerchantScoreManager()
{
    flush();
}

bool MerchantScoreManager::open(const std::string& scoreFilePath)
{
    scoreFilePath_ = scoreFilePath;
    MerchantScoreParser parser;

    if (! parser.parseFromFile(scoreFilePath_))
    {
        LOG(ERROR) << parser.errorMessage();
        return false;
    }

    if (! parser.warningMessage().empty())
    {
        LOG(WARNING) << "warnings in parsing merchant score:\n" << parser.warningMessage();
    }

    idScoreMap_.clear();

    const MerchantStrScoreMap& strScoreMap = parser.merchantStrScoreMap();
    setScore(strScoreMap);

    return true;
}

bool MerchantScoreManager::flush()
{
    if (scoreFilePath_.empty())
        return true;

    return flushScoreFile_() &&
           (!merchantValueTable_ || merchantValueTable_->flush()) &&
           (!categoryValueTable_ || categoryValueTable_->flush());
}

bool MerchantScoreManager::flushScoreFile_()
{
    MerchantStrScoreMap strScoreMap;
    getAllStrScore(strScoreMap);

    MerchantScoreRenderer renderer(strScoreMap);
    if (! renderer.renderToFile(scoreFilePath_))
    {
        LOG(ERROR) << "failed to write " << scoreFilePath_;
        return false;
    }

    return true;
}

void MerchantScoreManager::getAllStrScore(MerchantStrScoreMap& strScoreMap) const
{
    ScopedReadLock lock(mutex_);

    MerchantStrScoreMap::map_t& merchantStrMap = strScoreMap.map;
    const MerchantIdScoreMap::map_t& merchantIdMap = idScoreMap_.map;

    for (MerchantIdScoreMap::map_t::const_iterator merchantIt = merchantIdMap.begin();
        merchantIt != merchantIdMap.end(); ++merchantIt)
    {
        std::string merchantName;
        getMerchantName_(merchantIt->first, merchantName);
        CategoryStrScore& categoryStrScore = merchantStrMap[merchantName];

        getCategoryStrScore_(merchantIt->second, categoryStrScore);
    }
}

void MerchantScoreManager::getStrScore(
    const std::vector<std::string>& merchantNames,
    MerchantStrScoreMap& strScoreMap
) const
{
    ScopedReadLock lock(mutex_);

    MerchantStrScoreMap::map_t& merchantStrMap = strScoreMap.map;
    const MerchantIdScoreMap::map_t& merchantIdMap = idScoreMap_.map;

    for (std::vector<std::string>::const_iterator nameIt = merchantNames.begin();
        nameIt != merchantNames.end(); ++nameIt)
    {
        const std::string& merchantName = *nameIt;
        CategoryStrScore& categoryStrScore = merchantStrMap[merchantName];

        merchant_id_t merchantId = getMerchantId_(merchantName);
        MerchantIdScoreMap::map_t::const_iterator merchantIt = merchantIdMap.find(merchantId);

        if (merchantIt == merchantIdMap.end())
            continue;

        getCategoryStrScore_(merchantIt->second, categoryStrScore);
    }
}

score_t MerchantScoreManager::getIdScore(
    merchant_id_t merchantId,
    category_id_t categoryId
) const
{
    ScopedReadLock lock(mutex_);

    const MerchantIdScoreMap::map_t& merchantIdMap = idScoreMap_.map;
    MerchantIdScoreMap::map_t::const_iterator merchantIt = merchantIdMap.find(merchantId);

    if (merchantIt == merchantIdMap.end())
        return 0;

    const CategoryIdScore& categoryScore = merchantIt->second;
    const CategoryIdScore::CategoryScoreMap& categoryIdMap = categoryScore.categoryScoreMap;
    const CategoryIdScore::CategoryScoreMap::const_iterator categoryIt = categoryIdMap.find(categoryId);

    if (categoryIt == categoryIdMap.end())
        return categoryScore.generalScore;

    return categoryIt->second;
}

void MerchantScoreManager::setScore(const MerchantStrScoreMap& strScoreMap)
{
    ScopedWriteLock lock(mutex_);

    const MerchantStrScoreMap::map_t& merchantStrMap = strScoreMap.map;
    MerchantIdScoreMap::map_t& merchantIdMap = idScoreMap_.map;

    for (MerchantStrScoreMap::map_t::const_iterator merchantIt = merchantStrMap.begin();
        merchantIt != merchantStrMap.end(); ++merchantIt)
    {
        merchant_id_t merchantId = insertMerchantId_(merchantIt->first);

        if (merchantId == 0)
            continue;

        setCategoryIdScore_(merchantIt->second, merchantIdMap[merchantId]);
    }
}

void MerchantScoreManager::getCategoryStrScore_(
    const CategoryIdScore& categoryIdScore,
    CategoryStrScore& categoryStrScore
) const
{
    const CategoryIdScore::CategoryScoreMap& categoryIdMap = categoryIdScore.categoryScoreMap;
    CategoryStrScore::CategoryScoreMap& categoryStrMap = categoryStrScore.categoryScoreMap;

    categoryStrScore.generalScore = categoryIdScore.generalScore;

    for (CategoryIdScore::CategoryScoreMap::const_iterator categoryIt = categoryIdMap.begin();
        categoryIt != categoryIdMap.end(); ++categoryIt)
    {
        std::string categoryName;
        getCategoryName_(categoryIt->first, categoryName);

        categoryStrMap[categoryName] = categoryIt->second;
    }
}

void MerchantScoreManager::setCategoryIdScore_(
    const CategoryStrScore& categoryStrScore,
    CategoryIdScore& categoryIdScore
)
{
    CategoryIdScore::CategoryScoreMap& categoryIdMap = categoryIdScore.categoryScoreMap;
    const CategoryStrScore::CategoryScoreMap& categoryStrMap = categoryStrScore.categoryScoreMap;

    categoryIdScore.generalScore = categoryStrScore.generalScore;

    for (CategoryStrScore::CategoryScoreMap::const_iterator categoryIt = categoryStrMap.begin();
        categoryIt != categoryStrMap.end(); ++categoryIt)
    {
        category_id_t categoryId = insertCategoryId_(categoryIt->first);
        if (categoryId == 0)
            continue;

        categoryIdMap[categoryId] = categoryIt->second;
    }
}

merchant_id_t MerchantScoreManager::getMerchantId_(const std::string& merchant) const
{
    if (! merchantValueTable_)
        return 0;

    std::vector<izenelib::util::UString> path;
    path.push_back(izenelib::util::UString(merchant, ENCODING_TYPE));

    return merchantValueTable_->propValueId(path);
}

merchant_id_t MerchantScoreManager::insertMerchantId_(const std::string& merchant)
{
    if (! merchantValueTable_)
        return 0;

    merchant_id_t merchantId = 0;
    std::vector<izenelib::util::UString> path;
    path.push_back(izenelib::util::UString(merchant, ENCODING_TYPE));

    try
    {
        merchantId = merchantValueTable_->insertPropValueId(path);
    }
    catch(MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what() << ", merchant: " << merchant;
    }

    return merchantId;
}

category_id_t MerchantScoreManager::insertCategoryId_(const std::string& category)
{
    if (! categoryValueTable_)
        return 0;

    category_id_t categoryId = 0;
    std::vector<izenelib::util::UString> path;
    path.push_back(izenelib::util::UString(category, ENCODING_TYPE));

    try
    {
        categoryId = categoryValueTable_->insertPropValueId(path);
    }
    catch(MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what() << ", category: " << category;
    }

    return categoryId;
}

void MerchantScoreManager::getMerchantName_(merchant_id_t merchantId, std::string& merchant) const
{
    if (! merchantValueTable_)
        return;

    izenelib::util::UString ustr;
    merchantValueTable_->propValueStr(merchantId, ustr);

    ustr.convertString(merchant, ENCODING_TYPE);
}

void MerchantScoreManager::getCategoryName_(category_id_t categoryId, std::string& category) const
{
    if (! categoryValueTable_)
        return;

    izenelib::util::UString ustr;
    categoryValueTable_->propValueStr(categoryId, ustr);

    ustr.convertString(category, ENCODING_TYPE);
}

} // namespace sf1r
