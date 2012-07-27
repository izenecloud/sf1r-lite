/**
 * @file ProductRanker.h
 * @brief ranking products by below scores:
 * 1. custom score
 * 2. category score
 * 3. merchant score
 * 4. relevance score
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-05-17
 */

#ifndef SF1R_PRODUCT_RANKER_H
#define SF1R_PRODUCT_RANKER_H

#include "ProductScoreList.h"
#include <string>

namespace sf1r
{
class ProductRankingParam;
class ProductRankerFactory;

class ProductRanker
{
public:
    ProductRanker(
        ProductRankingParam& param,
        ProductRankerFactory& rankerFactory,
        bool isDebug
    );

    void rank();

private:
    void loadScore_();

    void sortScore_();

    void getResult_();

    void printMatrix_(const std::string& message) const;

private:
    ProductRankingParam& rankingParam_;
    ProductRankerFactory& rankerFactory_;

    ProductScoreMatrix scoreMatrix_;

    bool isDebug_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_RANKER_H
