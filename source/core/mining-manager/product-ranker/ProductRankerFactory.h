/**
 * @file ProductRankerFactory.h
 * @brief create product ranker instance.
 * @author Jun Jiang
 * @date Created 2012-11-30
 */

#ifndef SF1R_PRODUCT_RANKER_FACTORY_H
#define SF1R_PRODUCT_RANKER_FACTORY_H

namespace sf1r
{
struct ProductRankParam;
class ProductRanker;
class ProductRankingConfig;

namespace faceted { class PropValueTable; }

class ProductRankerFactory
{
public:
    ProductRankerFactory(
        const ProductRankingConfig& config,
        const faceted::PropValueTable* merchantValueTable);

    ProductRanker* createProductRanker(ProductRankParam& param);

private:
    void addCategoryEvaluator_(ProductRanker& ranker) const;
    void addRandomEvaluator_(ProductRanker& ranker) const;
    void addDiversityEvaluator_(ProductRanker& ranker) const;

private:
    const ProductRankingConfig& config_;

    const faceted::PropValueTable* merchantValueTable_;

    /** true when the weight in <Score type="random"> is non-zero */
    const bool isRandomScoreConfig_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_RANKER_FACTORY_H
