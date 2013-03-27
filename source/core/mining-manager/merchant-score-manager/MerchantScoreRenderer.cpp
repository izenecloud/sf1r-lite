#include "MerchantScoreRenderer.h"
#include <common/Keys.h>

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
            categoryValue[Keys::score] = categoryIt->second;

            Value& pathValue = categoryValue[Keys::category];
            const CategoryStrPath& strPath = categoryIt->first;
            for (CategoryStrPath::const_iterator nodeIt = strPath.begin();
                 nodeIt != strPath.end(); ++nodeIt)
            {
                pathValue() = *nodeIt;
            }
        }
    }
}

} // namespace sf1r
