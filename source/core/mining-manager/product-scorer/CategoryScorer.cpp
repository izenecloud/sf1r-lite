#include "CategoryScorer.h"
#include "../group-manager/PropValueTable.h"

using namespace sf1r;

namespace
{
/**
 * in order to make the category score less than 10 (the minimum custom
 * score), assuming at most 9 top labels are selected, we choose 1 as its
 * weight accordingly.
 */
const score_t kCategoryScoreWeight = 1;
}

CategoryScorer::CategoryScorer(
    const faceted::PropValueTable& categoryValueTable,
    const std::vector<category_id_t>& topLabels)
    : ProductScorer(kCategoryScoreWeight)
    , categoryValueTable_(categoryValueTable)
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
    CategoryScores::const_iterator it = categoryScores_.find(catId);

    if (it == categoryScores_.end())
        return 0;

    return it->second;
}
