/**
 * @file ProductRanker.h
 * @brief rank topK products by below scores:
 * 1. category score
 * 2. offer item count
 * 3. diversity round
 * @author Jun Jiang
 * @date Created 2012-11-30
 */

#ifndef SF1R_PRODUCT_RANKER_H
#define SF1R_PRODUCT_RANKER_H

#include "ProductScore.h"
#include <string>
#include <vector>

namespace sf1r
{
struct ProductRankParam;
class ProductScoreEvaluator;

class ProductRanker
{
public:
    ProductRanker(
        ProductRankParam& param,
        bool isDebug);

    ~ProductRanker();

    void addEvaluator(ProductScoreEvaluator* evaluator);

    void rank();

private:
    void loadScore_();

    void evaluateScore_(ProductScore& productScore);

    void sortScore_();

    void fetchResult_();

    void printScore_(const std::string& banner) const;

private:
    ProductRankParam& rankParam_;

    std::vector<ProductScoreEvaluator*> evaluators_;

    ProductScoreList scoreList_;

    bool isDebug_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_RANKER_H
