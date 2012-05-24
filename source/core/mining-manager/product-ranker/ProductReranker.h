/**
 * @file ProductReranker.h
 * @brief the interface to rerank products.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-05-17
 */

#ifndef SF1R_PRODUCT_RERANKER_H
#define SF1R_PRODUCT_RERANKER_H

#include "ProductScoreList.h"

namespace sf1r
{

class ProductReranker
{
public:
    virtual ~ProductReranker() {}

    virtual void rerank(ProductScoreMatrix& scoreMatrix) = 0;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_RERANKER_H
