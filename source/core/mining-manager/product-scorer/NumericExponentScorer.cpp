#include "NumericExponentScorer.h"
#include <common/NumericPropertyTableBase.h>
#include <util/fmath/fmath.hpp>

using namespace sf1r;

NumericExponentScorer::NumericExponentScorer(const ProductScoreConfig& config)
        : ProductScorer(config)
        , config_(config)
{
}

NumericExponentScorer::NumericExponentScorer(
    const ProductScoreConfig& config,
    boost::shared_ptr<NumericPropertyTableBase> numericTable,
    NumericPropertyNormalizer* normalizer)
        : ProductScorer(config)
        , config_(config)
        , numericTable_(numericTable)
        , normalizer_(normalizer)
{
}

score_t NumericExponentScorer::score(docid_t docId)
{
    score_t value = 0;

    if (numericTable_ && numericTable_->getFloatValue(docId, value))
    {
        if (normalizer_)
        {
            value = normalizer_->normalize(value);
        }

        return calculate(value);
    }

    return 0;
}

score_t NumericExponentScorer::calculate(score_t value) const
{
    config_.limitScore(value);

    score_t x = izenelib::util::fmath::exp(value / config_.zoomin);
    score_t score = (x - 1) / (x + 1);

    return score;
}
