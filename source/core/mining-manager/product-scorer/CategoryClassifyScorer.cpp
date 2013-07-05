#include "CategoryClassifyScorer.h"
#include "../category-classify/CategoryClassifyTable.h"

using namespace sf1r;

CategoryClassifyScorer::CategoryClassifyScorer(
    const ProductScoreConfig& config,
    const CategoryClassifyTable& categoryClassifyTable,
    const CategoryScoreMap& categoryScoreMap)
    : ProductScorer(config)
    , categoryClassifyTable_(categoryClassifyTable)
    , categoryScoreMap_(categoryScoreMap)
{
}

score_t CategoryClassifyScorer::score(docid_t docId)
{
    const std::string& category = categoryClassifyTable_.getCategoryNoLock(docId);
    CategoryScoreMap::const_iterator it = categoryScoreMap_.find(category);

    if (it != categoryScoreMap_.end())
        return it->second;

    return 0;
}
