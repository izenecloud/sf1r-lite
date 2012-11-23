/**
 * @file ProductScoreAverage.h
 * @brief It calculates the average value of multiple scores.
 *
 * It contains a list of ProductScorer, multiplies each score with its
 * weight, then calculates the average. That is, assuming there are N scores,
 * score = sum(score(i) * weight(i)) / N.
 *
 * @author Jun Jiang
 * @date Created 2012-11-23
 */

#ifndef SF1R_PRODUCT_SCORE_AVERAGE_H
#define SF1R_PRODUCT_SCORE_AVERAGE_H

#include "ProductScoreSum.h"

namespace sf1r
{

class ProductScoreAverage : public ProductScoreSum
{
public:
    ProductScoreAverage(const ProductScoreConfig& config);

    virtual score_t score(docid_t docId);
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_AVERAGE_H
