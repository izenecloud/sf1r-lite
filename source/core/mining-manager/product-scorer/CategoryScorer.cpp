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
    CategoryScores::const_iterator endIt = categoryScores_.end();

    for (category_id_t catId = categoryValueTable_.getFirstValueId(docId);
         catId; catId = parentIdTable_[catId])
    {
        CategoryScores::const_iterator it = categoryScores_.find(catId);

        if (it != endIt)
            return it->second;
    }

    return 0;
}
