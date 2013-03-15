#include "CategoryScorer.h"

using namespace sf1r;

CategoryScorer::CategoryScorer(
    const ProductScoreConfig& config,
    const faceted::PropValueTable& categoryValueTable,
    const std::vector<category_id_t>& topLabels)
    : ProductScorer(config)
    , categoryValueTable_(categoryValueTable)
    , parentIdTable_(categoryValueTable.parentIdTable())
{
    score_t score = 1;

    for (std::vector<category_id_t>::const_reverse_iterator
             rit = topLabels.rbegin(); rit != topLabels.rend(); ++rit)
    {
        categoryScores_[*rit] = score;
        ++score;
    }
}

score_t CategoryScorer::score(docid_t docId)
{
    category_id_t catId = categoryValueTable_.getFirstValueId(docId);
    categoryValueTable_.getParentIds(catId, parentIds_);
    CategoryScores::const_iterator endIt = categoryScores_.end();

    for (std::vector<category_id_t>::const_iterator parentIt = parentIds_.begin();
         parentIt != parentIds_.end(); ++parentIt)
    {
        CategoryScores::const_iterator findIt = categoryScores_.find(*parentIt);

        if (findIt != endIt)
            return findIt->second;
    }

    return 0;
}
