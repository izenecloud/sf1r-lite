#include "CategoryClassifyScorer.h"
#include "../category-classify/CategoryClassifyTable.h"
#include <iostream>

using namespace sf1r;

namespace
{
const score_t kReduceScoreForNotRule = 0.01;
}

const score_t CategoryClassifyScorer::kMinClassifyScore = 0.0001;

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
    {
        if (hasGoldCategory_ || "文娱>书籍杂志" == categoryRFlag.first || "文娱>音像影视" == categoryRFlag.first)
            return 0;
        return kMinClassifyScore;
    }

    score_t result = it->second;
    if (!categoryRFlag.second)
    {
        result -= kReduceScoreForNotRule;
    }
    if (result < kMinClassifyScore)
    {
        result = kMinClassifyScore;
    }

    int intResult = static_cast<int>(result / kMinClassifyScore);
    result = intResult * kMinClassifyScore;
    return result;
}
