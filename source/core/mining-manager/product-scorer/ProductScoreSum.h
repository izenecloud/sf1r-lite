/**
 * @file ProductScoreSum.h
 * @brief It sums up multiple scores.
 *
 * It contains a list of ProductScorer, sums up each score
 * multiplied by its weight, that is, score = sum(score(i) * weight(i)).
 *
 * @author Jun Jiang
 * @date Created 2012-10-26
 */

#ifndef SF1R_PRODUCT_SCORE_SUM_H
#define SF1R_PRODUCT_SCORE_SUM_H

#include "ProductScorer.h"
#include <vector>

namespace sf1r
{

class ProductScoreSum : public ProductScorer
{
public:
    ProductScoreSum() {}
    ProductScoreSum(const ProductScoreConfig& config);
    ~ProductScoreSum();

    void addScorer(ProductScorer* scorer);

    virtual score_t score(docid_t docId);

private:
    typedef std::vector<ProductScorer*> Scorers;
    Scorers scorers_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_SUM_H
