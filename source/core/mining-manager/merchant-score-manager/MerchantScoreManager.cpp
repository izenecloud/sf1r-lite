#include "MerchantScoreManager.h"
#include "MerchantScoreParser.h"
#include "MerchantScoreRenderer.h"
#include "../group-manager/PropValueTable.h"
#include "../util/convert_ustr.h"
#include "../MiningException.hpp"
#include <util/ustring/UString.h>
#include <3rdparty/febird/io/DataIO.h>
#include <3rdparty/febird/io/StreamBuffer.h>
#include <3rdparty/febird/io/FileStream.h>

#include <glog/logging.h>

namespace sf1r
{

MerchantScoreManager::MerchantScoreManager(
    faceted::PropValueTable* merchantValueTable,
    faceted::PropValueTable* categoryValueTable)
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
    ScopedWriteLock lock(mutex_);

    scoreFilePath_ = scoreFilePath;

    febird::FileStream ifs;
    if (! ifs.xopen(scoreFilePath_.c_str(), "r"))
        return true;

    try
    {
        febird::NativeDataInput<febird::InputBuffer> ar;
        ar.attach(&ifs);
        ar & idScoreMap_;
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception in febird::NativeDataInput: " << e.what()
                   << ", scoreFilePath_: " << scoreFilePath_;
        return false;
    }

    return true;
}

bool MerchantScoreManager::flush()
{
    return flushScoreFile_() &&
           (!merchantValueTable_ || merchantValueTable_->flush()) &&
           (!categoryValueTable_ || categoryValueTable_->flush());
}

bool MerchantScoreManager::flushScoreFile_()
{
    ScopedReadLock lock(mutex_);

    if (scoreFilePath_.empty())
        return true;

    febird::FileStream ofs(scoreFilePath_.c_str(), "w");
    if (! ofs)
    {
        LOG(ERROR) << "failed opening file " << scoreFilePath_;
        return false;
    }

    try
    {
        febird::NativeDataOutput<febird::OutputBuffer> ar;
        ar.attach(&ofs);
        ar & idScoreMap_;
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception in febird::NativeDataOutput: " << e.what()
                   << ", scoreFilePath_: " << scoreFilePath_;
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
    MerchantStrScoreMap& strScoreMap) const
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
    std::vector<category_id_t>& categoryParentIds) const
{
    ScopedReadLock lock(mutex_);

    const MerchantIdScoreMap::map_t& merchantIdMap = idScoreMap_.map;
    MerchantIdScoreMap::map_t::const_iterator merchantIt = merchantIdMap.find(merchantId);

    if (merchantIt == merchantIdMap.end())
        return 0;

    const CategoryIdScore& categoryScore = merchantIt->second;
    const CategoryIdScore::CategoryScoreMap& categoryIdMap = categoryScore.categoryScoreMap;

    typedef CategoryIdScore::CategoryScoreMap::const_iterator MapIter;
    const MapIter endIt = categoryIdMap.end();

    for (std::vector<category_id_t>::const_iterator parentIt = categoryParentIds.begin();
         parentIt != categoryParentIds.end(); ++parentIt)
    {
        MapIter findIt = categoryIdMap.find(*parentIt);

        if (findIt != endIt)
            return findIt->second;
    }

    return categoryScore.generalScore;
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
    CategoryStrScore& categoryStrScore) const
{
    const CategoryIdScore::CategoryScoreMap& categoryIdMap = categoryIdScore.categoryScoreMap;
    CategoryStrScore::CategoryScoreMap& categoryStrMap = categoryStrScore.categoryScoreMap;

    categoryStrScore.generalScore = categoryIdScore.generalScore;

    for (CategoryIdScore::CategoryScoreMap::const_iterator categoryIt = categoryIdMap.begin();
        categoryIt != categoryIdMap.end(); ++categoryIt)
    {
        CategoryStrPath categoryPath;
        getCategoryPath_(categoryIt->first, categoryPath);

        categoryStrMap[categoryPath] = categoryIt->second;
    }
}

void MerchantScoreManager::setCategoryIdScore_(
    const CategoryStrScore& categoryStrScore,
    CategoryIdScore& categoryIdScore)
{
    CategoryIdScore::CategoryScoreMap& categoryIdMap = categoryIdScore.categoryScoreMap;
    const CategoryStrScore::CategoryScoreMap& categoryStrMap = categoryStrScore.categoryScoreMap;

    categoryIdScore.generalScore = categoryStrScore.generalScore;
    categoryIdMap.clear();

    for (CategoryStrScore::CategoryScoreMap::const_iterator categoryIt =
             categoryStrMap.begin(); categoryIt != categoryStrMap.end(); ++categoryIt)
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

    std::vector<izenelib::util::UString> ustrPath(1);
    convert_to_ustr(merchant, ustrPath[0]);

    return merchantValueTable_->propValueId(ustrPath);
}

merchant_id_t MerchantScoreManager::insertMerchantId_(const std::string& merchant)
{
    if (! merchantValueTable_)
        return 0;

    merchant_id_t merchantId = 0;
    std::vector<izenelib::util::UString> ustrPath(1);
    convert_to_ustr(merchant, ustrPath[0]);

    try
    {
        merchantId = merchantValueTable_->insertPropValueId(ustrPath);
    }
    catch (MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what() << ", merchant: " << merchant;
    }

    return merchantId;
}

category_id_t MerchantScoreManager::insertCategoryId_(const CategoryStrPath& categoryPath)
{
    if (! categoryValueTable_)
        return 0;

    category_id_t categoryId = 0;
    std::vector<izenelib::util::UString> ustrPath;
    convert_to_ustr_vector(categoryPath, ustrPath);

    try
    {
        categoryId = categoryValueTable_->insertPropValueId(ustrPath);
    }
    catch (MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what();
    }

    return categoryId;
}

void MerchantScoreManager::getMerchantName_(
    merchant_id_t merchantId,
    std::string& merchant) const
{
    if (! merchantValueTable_)
        return;

    izenelib::util::UString ustr;
    merchantValueTable_->propValueStr(merchantId, ustr);

    convert_to_str(ustr, merchant);
}

void MerchantScoreManager::getCategoryPath_(
    category_id_t categoryId,
    CategoryStrPath& categoryPath) const
{
    if (! categoryValueTable_)
        return;

    std::vector<izenelib::util::UString> ustrPath;
    categoryValueTable_->propValuePath(categoryId, ustrPath);
    convert_to_str_vector(ustrPath, categoryPath);
}

} // namespace sf1r
