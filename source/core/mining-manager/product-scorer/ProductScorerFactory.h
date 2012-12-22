/**
 * @file ProductScorerFactory.h
 * @brief create ProductScorer instance.
 * @author Jun Jiang
 * @date Created 2012-10-28
 */

#ifndef SF1R_PRODUCT_SCORER_FACTORY_H
#define SF1R_PRODUCT_SCORER_FACTORY_H

#include "BoostLabelSelector.h"
#include <string>
#include <boost/scoped_ptr.hpp>

namespace sf1r
{
class ProductScorer;
class CustomRankManager;
class ProductRankingConfig;
class MiningManager;
class ProductScoreConfig;
class ProductScoreManager;
struct ProductScoreParam;

namespace faceted { class PropValueTable; }

class ProductScorerFactory
{
public:
    ProductScorerFactory(
        const ProductRankingConfig& config,
        MiningManager& miningManager);

    /**
     * create the @c ProductScorer instance, it sums up weighted scores
     * from @c CustomScorer, @c CategoryScorer, and @p relevanceScorer, etc.
     * @param scoreParam the parameters used to create @c ProductScorer instance.
     * @return the @c ProductScorer instance, it should be deleted by caller.
     */
    ProductScorer* createScorer(const ProductScoreParam& scoreParam);

private:
    ProductScorer* createScorerImpl_(
        const ProductScoreConfig& scoreConfig,
        const ProductScoreParam& scoreParam);

    ProductScorer* createCustomScorer_(
        const ProductScoreConfig& scoreConfig,
        const std::string& query);

    ProductScorer* createCategoryScorer_(
        const ProductScoreConfig& scoreConfig,
        const ProductScoreParam& scoreParam);

    ProductScorer* createRelevanceScorer_(
        const ProductScoreConfig& scoreConfig,
        ProductScorer* relevanceScorer);

    ProductScorer* createPopularityScorer_(
        const ProductScoreConfig& scoreConfig);

private:
    const ProductRankingConfig& config_;

    CustomRankManager* customRankManager_;

    const faceted::PropValueTable* categoryValueTable_;

    boost::scoped_ptr<BoostLabelSelector> labelSelector_;

    ProductScoreManager* productScoreManager_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORER_FACTORY_H
