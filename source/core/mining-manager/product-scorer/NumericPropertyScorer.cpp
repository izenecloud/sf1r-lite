#include "NumericPropertyScorer.h"
#include <common/NumericPropertyTableBase.h>

using namespace sf1r;

NumericPropertyScorer::NumericPropertyScorer(
    const ProductScoreConfig& config,
    boost::shared_ptr<NumericPropertyTableBase> numericTable)
    : ProductScorer(config)
    , numericTable_(numericTable)
{
}

score_t NumericPropertyScorer::score(docid_t docId)
{
    score_t score = 0;
    numericTable_->getFloatValue(docId, score);

    return score;
}
