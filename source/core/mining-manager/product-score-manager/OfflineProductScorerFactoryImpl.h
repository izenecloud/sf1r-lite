/**
 * @file OfflineProductScorerFactoryImpl.h
 * @brief implementation of creating ProductScorer instance for
 *        offline calculation.
 * @author Jun Jiang
 * @date Created 2012-11-16
 */

#ifndef SF1R_OFFLINE_PRODUCT_SCORER_FACTORY_IMPL_H
#define SF1R_OFFLINE_PRODUCT_SCORER_FACTORY_IMPL_H

#include "OfflineProductScorerFactory.h"

namespace sf1r
{
class NumericPropertyTableBuilder;

class OfflineProductScorerFactoryImpl : public OfflineProductScorerFactory
{
public:
    OfflineProductScorerFactoryImpl(
        NumericPropertyTableBuilder* numericTableBuilder);

    virtual ProductScorer* createScorer(
        const ProductScoreConfig& scoreConfig);

private:
    ProductScorer* createPopularityScorer_(
        const ProductScoreConfig& scoreConfig);

    ProductScorer* createNumericPropertyScorer_(
        const ProductScoreConfig& scoreConfig);

private:
    NumericPropertyTableBuilder* numericTableBuilder_;
};

} // namespace sf1r

#endif // SF1R_OFFLINE_PRODUCT_SCORER_FACTORY_IMPL_H
