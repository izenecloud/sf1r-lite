#include "MerchantScoreRenderer.h"
#include <common/Keys.h>

#include <fstream>

namespace sf1r
{
using driver::Keys;
using namespace izenelib::driver;

void MerchantScoreRenderer::renderToValue(izenelib::driver::Value& merchantArray) const
{
    const MerchantStrScoreMap::map_t& merchantStrMap = merchantStrScoreMap_.map;

    for (MerchantStrScoreMap::map_t::const_iterator merchantIt = merchantStrMap.begin();
        merchantIt != merchantStrMap.end(); ++merchantIt)
    {
        const CategoryStrScore& categoryStrScore = merchantIt->second;
        const CategoryStrScore::CategoryScoreMap& categoryStrMap = categoryStrScore.categoryScoreMap;

        Value& merchantValue = merchantArray();
        merchantValue[Keys::merchant] = merchantIt->first;
        merchantValue[Keys::score] = categoryStrScore.generalScore;

        Value& categoryArray = merchantValue[Keys::category_score];

        for (CategoryStrScore::CategoryScoreMap::const_iterator categoryIt = categoryStrMap.begin();
            categoryIt != categoryStrMap.end(); ++categoryIt)
        {
            Value& categoryValue = categoryArray();
            categoryValue[Keys::category] = categoryIt->first;
            categoryValue[Keys::score] = categoryIt->second;
        }
    }
}

bool MerchantScoreRenderer::renderToFile(const std::string& filePath) const
{
    std::ofstream ofs(filePath.c_str());
    if (! ofs)
        return false;

    const MerchantStrScoreMap::map_t& merchantStrMap = merchantStrScoreMap_.map;

    for (MerchantStrScoreMap::map_t::const_iterator merchantIt = merchantStrMap.begin();
        merchantIt != merchantStrMap.end(); ++merchantIt)
    {
        const CategoryStrScore& categoryStrScore = merchantIt->second;
        const CategoryStrScore::CategoryScoreMap& categoryStrMap = categoryStrScore.categoryScoreMap;

        ofs << merchantIt->first << "\t" << categoryStrScore.generalScore << std::endl;

        for (CategoryStrScore::CategoryScoreMap::const_iterator categoryIt = categoryStrMap.begin();
            categoryIt != categoryStrMap.end(); ++categoryIt)
        {
            ofs << categoryIt->first << "\t" << categoryIt->second << std::endl;
        }

        ofs << std::endl;
    }

    return true;
}

} // namespace sf1r
