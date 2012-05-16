#include "MerchantScoreParser.h"
#include <common/Keys.h>

#include <fstream>
#include <sstream>
#include <boost/algorithm/string/trim.hpp>

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
    CategoryStrScore& categoryStrScore
)
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

        std::string categoryName = asString(itemValue[Keys::category]);
        if (categoryName.empty())
        {
            error() = "Require non-empty value for request[resource][category_score][category].";
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

        categoryMap[categoryName] = score;
    }

    return true;
}

bool MerchantScoreParser::parseFromFile(const std::string& filePath)
{
    std::ifstream ifs(filePath.c_str());
    if (! ifs)
        return true;

    std::string line;
    MerchantStrScoreMap::map_t& merchantMap = merchantStrScoreMap_.map;

    while (getline(ifs, line))
    {
        boost::algorithm::trim(line);

        if (line.empty())
            continue;

        std::istringstream iss(line);
        std::string merchant;
        score_t score = 0;
        iss >> merchant >> score;

        if (merchant.empty())
        {
            warning() += "- Require non-empty merchant name\n";
            continue;
        }

        if (score < 0)
        {
            warning() += "- the negative score of merchant " + merchant + " is adjusted to zero\n";
            score = 0;
        }

        CategoryStrScore& categoryStrScore = merchantMap[merchant];
        categoryStrScore.generalScore = score;

        if (!parseCategoryScoreFromStream_(ifs, categoryStrScore))
            return false;
    }

    return true;
}

bool MerchantScoreParser::parseCategoryScoreFromStream_(
    std::istream& ist,
    CategoryStrScore& categoryStrScore
)
{
    std::string line;
    CategoryStrScore::CategoryScoreMap& categoryMap = categoryStrScore.categoryScoreMap;

    while (getline(ist, line))
    {
        boost::algorithm::trim(line);

        // empty line means the end of current merchant
        if (line.empty())
            return true;

        std::istringstream iss(line);
        std::string category;
        score_t score = 0;
        iss >> category >> score;

        if (category.empty())
        {
            warning() += "- Require non-empty category name\n";
            continue;
        }

        if (score < 0)
        {
            warning() += "- the negative score of category " + category + " is adjusted to zero\n";
            score = 0;
        }

        categoryMap[category] = score;
    }

    return true;
}

} // namespace sf1r
