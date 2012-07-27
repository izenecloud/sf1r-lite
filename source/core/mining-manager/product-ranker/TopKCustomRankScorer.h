/**
 * @file TopKCustomRankScorer.h
 * @brief get custom ranking score for top-K docs.
 *        for customized doc id, it gives positive score;
 *        otherwise, it gives zero score.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-07-19
 */

#ifndef SF1R_TOPK_CUSTOM_RANK_SCORER_H
#define SF1R_TOPK_CUSTOM_RANK_SCORER_H

#include "ProductScorer.h"

namespace sf1r
{

class TopKCustomRankScorer : public ProductScorer
{
public:
    TopKCustomRankScorer();

    virtual void pushScore(
        const ProductRankingParam& param,
        ProductScoreMatrix& scoreMatrix
    );
};

} // namespace sf1r

#endif // SF1R_TOPK_CUSTOM_RANK_SCORER_H
