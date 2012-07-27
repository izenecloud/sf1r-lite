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
    ProductScorer(const std::string& scoreMessage, int scoreNum = 1)
        : scoreMessage_(scoreMessage)
        , scoreNum_(scoreNum)
    {}

    virtual ~ProductScorer() {}

    const std::string& getScoreMessage() const { return scoreMessage_; }

    int getScoreNum() const { return scoreNum_; }

    virtual void pushScore(
        const ProductRankingParam& param,
        ProductScoreMatrix& scoreMatrix
    ) = 0;

protected:
    std::string scoreMessage_;
    const int scoreNum_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORER_H
