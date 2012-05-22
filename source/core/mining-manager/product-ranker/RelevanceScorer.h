/**
 * @file RelevanceScorer.h
 * @brief get relevance score.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-05-17
 */

#ifndef SF1R_RELEVANCE_SCORER_H
#define SF1R_RELEVANCE_SCORER_H

#include "ProductScorer.h"

namespace sf1r
{

class RelevanceScorer : public ProductScorer
{
public:
    virtual void pushScore(
        const ProductRankingParam& param,
        ProductScoreMatrix& scoreMatrix
    );
};

} // namespace sf1r

#endif // SF1R_RELEVANCE_SCORER_H
