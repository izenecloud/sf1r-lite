/**
 * @file ProductScorerFactory.h
 * @brief create ProductScorer instance.
 * @author Jun Jiang
 * @date Created 2012-10-28
 */

#ifndef SF1R_PRODUCT_SCORER_FACTORY_H
#define SF1R_PRODUCT_SCORER_FACTORY_H

#include <string>

namespace sf1r
{
class ProductScorer;
class CustomRankManager;
class GroupLabelLogger;
class ProductRankingConfig;
class MiningManager;

namespace faceted
{
class PropValueTable;
class PropSharedLockSet;
}

class ProductScorerFactory
{
public:
    ProductScorerFactory(
        const ProductRankingConfig& config,
        MiningManager& miningManager);

    /**
     * create the @c ProductScorer instance, it sums up weighted scores
     * from @c CustomScorer, @c CategoryScorer, and @p relevanceScorer, etc.
     * @param query used to create @c CustomScorer and @c CategoryScorer.
     * @param propSharedLockSet used to concurrently access category data
     *        by @c CategoryScorer.
     * @param relevanceScorer if not NULL, it would be added into return value.
     * @return the @c ProductScorer instance, it should be deleted by caller.
     */
    ProductScorer* createScorer(
        const std::string& query,
        faceted::PropSharedLockSet& propSharedLockSet,
        ProductScorer* relevanceScorer);

private:
    ProductScorer* createCustomScorer_(const std::string& query);

    ProductScorer* createCategoryScorer_(
        const std::string& query,
        faceted::PropSharedLockSet& propSharedLockSet);

private:
    CustomRankManager* customRankManager_;

    GroupLabelLogger* categoryClickLogger_;

    const faceted::PropValueTable* categoryValueTable_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORER_FACTORY_H
