/**
 * @file NumericPropertyScorer.h
 * @brief It returns a score for property's numeric value.
 *
 * The score range is [0, 1], which is calculated like below:
 * score(i) = (value(i) - min_value) / (max_value - min_value)
 *
 * @author Jun Jiang
 * @date Created 2012-11-26
 */

#ifndef SF1R_NUMERIC_PROPERTY_SCORER_H
#define SF1R_NUMERIC_PROPERTY_SCORER_H

#include "ProductScorer.h"
#include <boost/shared_ptr.hpp>

namespace sf1r
{
class NumericPropertyTableBase;

class NumericPropertyScorer : public ProductScorer
{
public:
    NumericPropertyScorer(
        const ProductScoreConfig& config,
        boost::shared_ptr<NumericPropertyTableBase> numericTable);

    virtual score_t score(docid_t docId);

private:
    boost::shared_ptr<NumericPropertyTableBase> numericTable_;
    score_t minValue_;
    score_t maxValue_;
    score_t minMaxDiff_;
};

} // namespace sf1r

#endif // SF1R_NUMERIC_PROPERTY_SCORER_H
