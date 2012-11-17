/**
 * @file ProductScorer.h
 * @brief the interface to calculate score for each product.
 * @author Jun Jiang
 * @date Created 2012-10-24
 */

#ifndef SF1R_PRODUCT_SCORER_H
#define SF1R_PRODUCT_SCORER_H

#include <common/inttypes.h>
#include <configuration-manager/ProductScoreConfig.h>

namespace sf1r
{

class ProductScorer
{
public:
    ProductScorer(score_t weight = 1.0)
        : weight_(weight)
    {}

    ProductScorer(const ProductScoreConfig& config)
        : weight_(config.weight)
    {}

    virtual ~ProductScorer() {}

    score_t weight() const { return weight_; }

    void setWeight(score_t weight) { weight_ = weight; }

    virtual score_t score(docid_t docId) = 0;

protected:
    score_t weight_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORER_H
