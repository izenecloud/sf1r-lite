#include "CategoryClassifyScorer.h"
#include "../category-classify/CategoryClassifyTable.h"
#include <iostream>

using namespace sf1r;

namespace
{
const score_t kReduceScoreForNotRule = 0.01;
const score_t kMinClassifyScore = 0.0001;
}

CategoryClassifyScorer::CategoryClassifyScorer(
    const ProductScoreConfig& config,
    const CategoryClassifyTable& categoryClassifyTable,
    const CategoryScoreMap& categoryScoreMap)
    : ProductScorer(config)
    , categoryClassifyTable_(categoryClassifyTable)
    , categoryScoreMap_(categoryScoreMap)
    , hasGoldCategory_(false)
{
    for (CategoryScoreMap::const_iterator it = categoryScoreMap_.begin();
         it != categoryScoreMap_.end(); ++it)
    {
        if (it->second == 1)
        {
            hasGoldCategory_ = true;
            break;
        }
    }
}

score_t CategoryClassifyScorer::score(docid_t docId)
{
    const CategoryClassifyTable::category_rflag_t& categoryRFlag =
        categoryClassifyTable_.getCategoryNoLock(docId);

    CategoryScoreMap::const_iterator it =
        categoryScoreMap_.find(categoryRFlag.first);

    if (it == categoryScoreMap_.end())
        return hasGoldCategory_ ? 0 : kMinClassifyScore;

    score_t result = it->second;
    if (!categoryRFlag.second)
    {
        result -= kReduceScoreForNotRule;
    }
    if (result < kMinClassifyScore)
    {
        result = kMinClassifyScore;
    }
    int int_res = static_cast<int>(result*10000);
    score_t re_result = (double(int_res))/10000.0;
    return re_result;
}
