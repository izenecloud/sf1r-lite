/**
 * @file NumericExponentScorer.h
 * @brief It returns a score for property's numeric value.
 *
 * The score range is [0, 1], which is calculated like below:
 * x = exp(value(i) / zoomin)
 * score(i) = (x - 1) / (x + 1)
 */

#ifndef SF1R_NUMERIC_EXPONENT_SCORER_H
#define SF1R_NUMERIC_EXPONENT_SCORER_H

#include "ProductScorer.h"
#include <boost/shared_ptr.hpp>

namespace sf1r
{
class NumericPropertyTableBase;

class NumericExponentScorer : public ProductScorer
{
public:
    NumericExponentScorer(const ProductScoreConfig& config);

    NumericExponentScorer(
        const ProductScoreConfig& config,
        boost::shared_ptr<NumericPropertyTableBase> numericTable);

    virtual score_t score(docid_t docId);

    score_t calculate(score_t value) const;

private:
    const ProductScoreConfig& config_;
    boost::shared_ptr<NumericPropertyTableBase> numericTable_;
};

} // namespace sf1r

#endif // SF1R_NUMERIC_EXPONENT_SCORER_H
