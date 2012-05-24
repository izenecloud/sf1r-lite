/**
 * @file ProductScorer.h
 * @brief the interface to calculate score for each product.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-05-17
 */

#ifndef SF1R_PRODUCT_SCORER_H
#define SF1R_PRODUCT_SCORER_H

#include "ProductRankingParam.h"
#include "ProductScoreList.h"

#include <string>

namespace sf1r
{

class ProductScorer
{
public:
    ProductScorer(const std::string& scoreMessage)
        : scoreMessage_(scoreMessage)
    {}

    virtual ~ProductScorer() {}

    const std::string& getScoreMessage() const { return scoreMessage_; }

    virtual void pushScore(
        const ProductRankingParam& param,
        ProductScoreMatrix& scoreMatrix
    ) = 0;

private:
    const std::string scoreMessage_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORER_H
