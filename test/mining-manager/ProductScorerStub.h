/**
 * @file ProductScorerStub.h
 * @brief stub class to return a specified score.
 * @author Jun Jiang
 * @date Created 2012-11-23
 */

#ifndef SF1R_PRODUCT_SCORER_STUB_H
#define SF1R_PRODUCT_SCORER_STUB_H

#include <mining-manager/product-scorer/ProductScorer.h>

namespace sf1r
{

class ProductScorerStub : public ProductScorer
{
public:
    ProductScorerStub(const ProductScoreConfig& config, score_t score)
        : ProductScorer(config)
        , score_(score)
    {}

    virtual score_t score(docid_t docId) { return score_; }

private:
    const score_t score_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORER_STUB_H
