/**
 * @file ProductRankerFactory.h
 * @brief create product ranker instance.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-05-17
 */

#ifndef SF1R_PRODUCT_RANKER_FACTORY_H
#define SF1R_PRODUCT_RANKER_FACTORY_H

#include "ProductRankingParam.h"
#include <vector>

namespace sf1r
{
class ProductScorer;
class ProductRanker;
class ProductReranker;
class CategoryBoostingScorer;
class MiningManager;
class ProductRankingConfig;
class MerchantScoreManager;

namespace faceted
{
class PropValueTable;
}

class ProductRankerFactory
{
public:
    ProductRankerFactory(MiningManager* miningManager);
    ~ProductRankerFactory();

    ProductRanker* createProductRanker(ProductRankingParam& param);

    std::vector<ProductScorer*>& getScorers() { return scorers_; }

    enum RerankerType
    {
        DIVERSITY_RERANKER = 0,
        CTR_RERANKER,
        RERANKER_TYPE_NUM
    };

    ProductReranker* getReranker(RerankerType type) { return rerankers_[type]; }

private:
    void createCategoryBoostingScorer_();

    void createMerchantScorer_();

    void createRelevanceScorer_();

    void createCTRReranker_();

private:
    MiningManager* miningManager_;
    const ProductRankingConfig& config_;

    const faceted::PropValueTable* categoryValueTable_;
    const faceted::PropValueTable* merchantValueTable_;
    const MerchantScoreManager* merchantScoreManager_;

    std::vector<ProductScorer*> scorers_;
    std::vector<ProductReranker*> rerankers_;

    CategoryBoostingScorer* boostingScorer_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_RANKER_FACTORY_H
