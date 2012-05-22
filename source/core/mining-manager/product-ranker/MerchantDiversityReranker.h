/**
 * @file MerchantDiversityReranker.h
 * @brief rerank products by merchant diversity.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-05-22
 */

#ifndef SF1R_MERCHANT_DIVERSITY_RERANKER_H
#define SF1R_MERCHANT_DIVERSITY_RERANKER_H

#include "ProductReranker.h"

namespace sf1r
{

class MerchantDiversityReranker : public ProductReranker
{
public:
    MerchantDiversityReranker(int merchantCountIndex);

    virtual void rerank(ProductScoreMatrix& scoreMatrix);

private:
    ProductScoreRange findDiversityRange_(
        ProductScoreIter first,
        ProductScoreIter last
    ) const;

private:
    const int merchantCountIndex_;
};

} // namespace sf1r

#endif // SF1R_MERCHANT_DIVERSITY_RERANKER_H
