/**
 * @file PopularityScorer.h
 * @brief The popularity score shows how popular each product is.
 *
 * 1. it contains a list of ProductScorer.
 * 2. it sums up each score multiplied by its weight,
 *    that is, sum(score(i) * weight(i)).
 * 3. it normalizes the above sum into range [0, 1].
 *
 * @author Jun Jiang
 * @date Created 2012-11-14
 */

#ifndef SF1R_POPULARITY_SCORE_H
#define SF1R_POPULARITY_SCORE_H

#include "ProductScoreSum.h"

namespace sf1r
{

class PopularityScorer : public ProductScoreSum
{
public:
    PopularityScorer(const ProductScoreConfig& config);

    virtual score_t score(docid_t docId);
};

} // namespace sf1r

#endif // SF1R_POPULARITY_SCORE_H
