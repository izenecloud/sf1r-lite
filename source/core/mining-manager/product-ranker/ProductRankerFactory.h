/**
 * @file ProductRankerFactory.h
 * @brief create product ranker instance.
 * @author Jun Jiang
 * @date Created 2012-11-30
 */

#ifndef SF1R_PRODUCT_RANKER_FACTORY_H
#define SF1R_PRODUCT_RANKER_FACTORY_H

#include <boost/shared_ptr.hpp>

namespace sf1r
{
struct ProductRankParam;
class ProductRanker;
class ProductRankingConfig;
class MerchantScoreManager;
class NumericPropertyTableBase;

namespace faceted { class PropValueTable; }

class ProductRankerFactory
{
public:
    ProductRankerFactory(
        const ProductRankingConfig& config,
        const faceted::PropValueTable* categoryValueTable,
        const boost::shared_ptr<const NumericPropertyTableBase>& offerItemCountTable,
        const faceted::PropValueTable* diversityValueTable,
        const MerchantScoreManager* merchantScoreManager);

    ProductRanker* createProductRanker(ProductRankParam& param);

private:
    void addCategoryEvaluator_(ProductRanker& ranker, bool isLongQuery) const;
    void addRandomEvaluator_(ProductRanker& ranker) const;
    void addOfferItemCountEvaluator_(ProductRanker& ranker, bool isLongQuery) const;
    void addDiversityEvaluator_(ProductRanker& ranker) const;
    void addMerchantScoreEvaluator_(ProductRanker& ranker) const;

private:
    const ProductRankingConfig& config_;

    const faceted::PropValueTable* categoryValueTable_;
    boost::shared_ptr<const NumericPropertyTableBase> offerItemCountTable_;
    const faceted::PropValueTable* diversityValueTable_;
    const MerchantScoreManager* merchantScoreManager_;

    /** true when the weight in <Score type="random"> is non-zero */
    const bool isRandomScoreConfig_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_RANKER_FACTORY_H
