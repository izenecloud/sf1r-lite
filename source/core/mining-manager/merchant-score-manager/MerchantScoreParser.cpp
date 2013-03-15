#include "MerchantScoreParser.h"
#include <common/Keys.h>
#include <string>

namespace sf1r
{
using driver::Keys;
using namespace izenelib::driver;

bool MerchantScoreParser::parse(const izenelib::driver::Value& merchantArray)
{
    if (nullValue(merchantArray) ||
        merchantArray.type() != Value::kArrayType)
    {
        error() = "Require an array of merchant scores for request[resource].";
        return false;
    }

    MerchantStrScoreMap::map_t& merchantMap = merchantStrScoreMap_.map;

    for (std::size_t i = 0; i < merchantArray.size(); ++i)
    {
        const Value& itemValue = merchantArray(i);

        std::string merchantName = asString(itemValue[Keys::merchant]);
        if (merchantName.empty())
        {
            error() = "Require non-empty value for request[resource][merchant].";
            return false;
        }

        const Value& scoreValue = itemValue[Keys::score];
        if (nullValue(scoreValue))
        {
            error() = "Require non-empty value for request[resource][score].";
            return false;
        }

        float generalScore = asDouble(scoreValue);
        if (generalScore < 0)
        {
            error() = "Require non-negative value for request[resource][score].";
            return false;
        }

        CategoryStrScore& categoryStrScore = merchantMap[merchantName];
        categoryStrScore.generalScore = generalScore;

        if (! parseCategoryScoreFromValue_(itemValue[Keys::category_score], categoryStrScore))
            return false;
    }

    return true;
}

bool MerchantScoreParser::parseCategoryScoreFromValue_(
    const izenelib::driver::Value& categoryArray,
    CategoryStrScore& categoryStrScore)
{
    if (nullValue(categoryArray))
        return true;

    if (categoryArray.type() != Value::kArrayType)
    {
        error() = "Require an array of category scores for request[resource][category_score].";
        return false;
    }

    CategoryStrScore::CategoryScoreMap& categoryMap = categoryStrScore.categoryScoreMap;

    for (std::size_t i = 0; i < categoryArray.size(); ++i)
    {
        const Value& itemValue = categoryArray(i);
        const Value& categoryValue = itemValue[Keys::category];

        if (nullValue(categoryValue) ||
            categoryValue.type() != Value::kArrayType)
        {
            error() = "Require an array for request[resource][category_score][category].";
            return false;
        }

        CategoryStrPath categoryPath;
        for (std::size_t j = 0; j < categoryValue.size(); ++j)
        {
            std::string categoryName = asString(categoryValue(j));
            categoryPath.push_back(categoryName);
        }

        if (categoryPath.empty())
        {
            error() = "Require non-empty array for request[resource][category_score][category].";
            return false;
        }

        const Value& scoreValue = itemValue[Keys::score];
        if (nullValue(scoreValue))
        {
            error() = "Require non-empty value for request[resource][category_score][score].";
            return false;
        }

        float score = asDouble(scoreValue);
        if (score < 0)
        {
            error() = "Require non-negative value for request[resource][category_score][score].";
            return false;
        }

        categoryMap[categoryPath] = score;
    }

    return true;
}

} // namespace sf1r
